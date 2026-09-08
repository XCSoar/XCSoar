// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Update/Backend.hpp"
#include "Update/RepositoryOffer.hpp"
#include "Update/Service.hpp"
#include "Update/State.hpp"
#include "Repository/FileRepository.hpp"
#include "Profile/Current.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "Profile/ProfileMap.hpp"
#include "TestUtil.hpp"
#include "util/UriUtil.hpp"

#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

#include <boost/json.hpp>

class FakeUpdateBackend final : public UpdateBackend {
public:
  int priority = 0;
  unsigned checks = 0;
  bool shutdown = false;
  bool attached = false;
  bool throw_check = false;
  bool throw_consume = false;
  std::optional<UpdateCheckResult> result;

  UpdateBackendId GetId() const noexcept override {
    return UpdateBackendId::REPOSITORY;
  }
  int GetPriority() const noexcept override { return priority; }
  bool SupportsCheck(UpdateCheckTrigger) const noexcept override {
    return true;
  }
  void OnAttached() noexcept override { attached = true; }

  void StartCheck(UpdateCheckTrigger) override {
    if (throw_check)
      throw std::runtime_error("check failed");
    ++checks;
  }
  std::optional<UpdateCheckResult> ConsumeCheckResult() override {
    if (throw_consume)
      throw std::runtime_error("consume failed");
    return std::exchange(result, std::nullopt);
  }
  void BeginShutdown() noexcept override { shutdown = true; }
};

static UpdateCheckResult
MakeAvailable(const char *version)
{
  if (version == nullptr || *version == '\0')
    throw std::invalid_argument("empty version");

  UpdateCheckResult result;
  result.state = UpdateState::AVAILABLE;
  result.info.emplace();
  result.info->backend_id = UpdateBackendId::REPOSITORY;
  result.info->offer_id = version;
  result.info->version = version;
  return result;
}

static void
TestSchedule()
{
  Profile::Clear();
  UpdateStateData state;
  ok1(state.IsAutomaticCheckDue(100000));

  state.last_successful_check = 100000;
  ok1(!state.IsAutomaticCheckDue(100000 + 86399));
  ok1(state.IsAutomaticCheckDue(100000 + 86400));
  ok1(state.IsAutomaticCheckDue(99999));

  Profile::Set(ProfileKeys::UpdateCheckEnabled, true);
  bool enabled = false;
  Profile::Get(ProfileKeys::UpdateCheckEnabled, enabled);
  ok1(enabled);
  Profile::Set(ProfileKeys::UpdateCheckEnabled, false);
  Profile::Get(ProfileKeys::UpdateCheckEnabled, enabled);
  ok1(!enabled);

  state.Dismiss(UpdateBackendId::REPOSITORY, "2026.1");
  ok1(state.IsDismissed(UpdateBackendId::REPOSITORY, "2026.1"));
  ok1(!state.IsDismissed(UpdateBackendId::REPOSITORY, "2026.2"));
}

static void
TestAutomaticCheckDefault()
{
  Profile::Clear();
  UpdateService service;
  FakeUpdateBackend backend;
  service.Attach(backend);

  ok1(service.StartAutomaticCheck(100000));
  ok1(backend.checks == 1);
}

static void
TestOwnedBackend()
{
  UpdateService service;
  auto backend = std::make_unique<FakeUpdateBackend>();
  FakeUpdateBackend *const raw_backend = backend.get();

  service.AddBackend(std::move(backend));
  ok1(!backend);
  ok1(raw_backend->attached);
  ok1(service.HasBackend());

  service.BeginShutdown();
  ok1(raw_backend->shutdown);
}

static void
TestStateJSON()
{
  UpdateStateData state;
  state.last_successful_check = 1787680800;
  state.Dismiss(UpdateBackendId::REPOSITORY, "2026.1");

  const auto parsed = ParseUpdateStateJSON(MakeUpdateStateJSON(state));
  ok1(parsed.has_value());
  ok1(parsed->last_successful_check == state.last_successful_check);
  ok1(parsed->dismissed_offers == state.dismissed_offers);

  ok1(ParseUpdateStateJSON(boost::json::parse(R"({"schema":2})"))
        .has_value());
  ok1(!ParseUpdateStateJSON(boost::json::parse(R"({"schema":3})")));
  ok1(!ParseUpdateStateJSON(boost::json::parse(
    R"({"schema":1,"last_successful_check":-1})")));
  ok1(!ParseUpdateStateJSON(boost::json::parse(
    R"({"schema":1,"dismissed_offer":"other-v1/2026.1"})")));
  ok1(!ParseUpdateStateJSON(boost::json::parse(
    R"({"schema":1,"dismissed_offer":"repository-v1/2026/1"})")));
  ok1(!ParseUpdateStateJSON(boost::json::parse(
    R"({"schema":1,"dismissed_offer":"repository-v1/2026 1"})")));
}

static std::optional<UpdateStateData> saved_state;

static void
SaveStateForTest(const UpdateStateData &state) noexcept
{
  saved_state = state;
}

static void
TestService()
{
  Profile::Clear();
  saved_state.reset();
  UpdateService service({}, SaveStateForTest);
  FakeUpdateBackend backend;
  service.Attach(backend);
  service.OnStartupFinished();

  ok1(service.StartCheck());
  ok1(backend.checks == 1);
  ok1(!service.StartCheck());

  UpdateCheckResult result;
  result.state = UpdateState::AVAILABLE;
  result.info.emplace();
  result.info->backend_id = UpdateBackendId::REPOSITORY;
  result.info->offer_id = "2026.1";
  backend.result = result;
  service.OnBackendCompletion();
  ok1(service.GetState() == UpdateState::AVAILABLE);
  ok1(service.GetInfo() != nullptr);
  ok1(saved_state && saved_state->last_successful_check.has_value());
  ok1(service.GetLastSuccessfulCheck() ==
      saved_state->last_successful_check);

  service.BeginShutdown();
  ok1(!service.StartCheck());
}

static void
TestCachedResult()
{
  Profile::Clear();
  Profile::Set(ProfileKeys::UpdateCheckEnabled, true);
  saved_state.reset();
  UpdateService service({}, SaveStateForTest);
  FakeUpdateBackend backend;
  service.Attach(backend);

  UpdateCheckResult cached;
  cached.state = UpdateState::AVAILABLE;
  cached.origin = UpdateResultOrigin::CACHE;
  cached.info.emplace();
  cached.info->backend_id = UpdateBackendId::REPOSITORY;
  cached.info->offer_id = "2026.1";
  backend.result = cached;
  service.OnBackendCompletion();
  ok1(service.GetState() == UpdateState::AVAILABLE);
  ok1(service.ShouldNotifyAutomatically());

  service.OnStartupFinished();
  ok1(service.GetState() == UpdateState::AVAILABLE);
  ok1(!saved_state);

  ok1(service.StartCheck(true));
  UpdateCheckResult error;
  error.state = UpdateState::FAILED;
  backend.result = error;
  service.OnBackendCompletion();
  ok1(service.GetState() == UpdateState::AVAILABLE);
  ok1(service.GetInfo() != nullptr);
  ok1(!service.ShouldNotifyAutomatically());
  ok1(!saved_state);
}

static void
TestStartupCompletion()
{
  Profile::Clear();
  UpdateService service;
  FakeUpdateBackend backend;
  service.Attach(backend);
  unsigned notifications = 0;
  service.SetResultListener([&notifications] { ++notifications; });

  ok1(service.StartCheck());
  UpdateCheckResult result;
  result.state = UpdateState::AVAILABLE;
  result.info.emplace();
  backend.result = result;
  service.OnBackendCompletion();
  ok1(service.GetState() == UpdateState::AVAILABLE);
  ok1(notifications == 0);

  service.OnStartupFinished();
  ok1(service.GetState() == UpdateState::AVAILABLE);
  ok1(notifications == 1);
}

static void
TestThrowingResultListener()
{
  UpdateService service;
  bool throw_listener = true;
  service.SetResultListener([&throw_listener] {
    if (throw_listener)
      throw std::runtime_error("listener failed");
  });
  ok1(!service.StartCheck());
}

static void
TestBackendFailures()
{
  Profile::Clear();
  UpdateService service;
  FakeUpdateBackend backend;
  service.Attach(backend);
  service.OnStartupFinished();

  backend.throw_check = true;
  ok1(!service.StartCheck());
  ok1(service.GetState() == UpdateState::FAILED);

  backend.throw_check = false;
  backend.throw_consume = true;
  ok1(service.StartCheck());
  service.OnBackendCompletion();
  ok1(service.GetState() == UpdateState::FAILED);
}

static void
TestMultipleBackends()
{
  Profile::Clear();
  UpdateService service;
  FakeUpdateBackend primary;
  FakeUpdateBackend preferred;
  preferred.priority = 100;
  service.Attach(primary);
  service.Attach(preferred);
  service.OnStartupFinished();

  ok1(service.StartCheck());
  ok1(primary.checks == 1);
  ok1(preferred.checks == 1);

  primary.result = MakeAvailable("8.0");
  service.OnBackendCompletion();
  ok1(service.GetInfo() != nullptr);
  ok1(service.GetInfo()->version.equals("8.0"));

  preferred.result = MakeAvailable("8.1");
  service.OnBackendCompletion();
  ok1(service.GetInfo() != nullptr);
  ok1(service.GetInfo()->version.equals("8.1"));

  preferred.result.emplace();
  preferred.result->state = UpdateState::UP_TO_DATE;
  service.OnBackendCompletion();
  ok1(service.GetInfo() != nullptr);
  ok1(service.GetInfo()->version.equals("8.0"));
}

static AvailableFile
MakeRepositoryOffer(const char *version = "2026.1")
{
  AvailableFile file;
  file.Clear();
  file.name = "xcsoar-UNIX";
  file.uri = "https://xcsoar.org/download";
  file.description = "Summary";
  file.type = FileType::SOFTWARE_UPDATE;
  file.software_update.emplace();
  file.software_update->target = "UNIX";
  file.software_update->version = version;
  file.software_update->channel = "stable";
  file.software_update->offer_id = version;
  file.software_update->source = "XCSoar";
  return file;
}

static void
TestRepositoryOffer()
{
  static constexpr std::string_view ALLOWED_HOSTS = "xcsoar.org";
  FileRepository repository;
  repository.files.emplace_back(MakeRepositoryOffer());

  const auto result = UpdateRepository::FindOffer(repository, "UNIX",
                                                   ALLOWED_HOSTS, "2025.4");
  ok1(result.has_value());
  ok1(result->state == UpdateState::AVAILABLE);
  ok1(result->info->handoff_url.equals("https://xcsoar.org/download"));
  ok1(result->info->summary.equals("Summary"));
  ok1(result->info->scope == UpdateScope::UNKNOWN);
  ok1(result->info->restart == RestartRequirement::UNKNOWN);

  const auto same = UpdateRepository::FindOffer(repository, "UNIX",
                                                 ALLOWED_HOSTS, "2026.1");
  ok1(same && same->state == UpdateState::UP_TO_DATE && !same->info);
  const auto older_offer = UpdateRepository::FindOffer(repository, "UNIX",
                                                        ALLOWED_HOSTS,
                                                        "2026.1.1");
  ok1(older_offer && older_offer->state == UpdateState::UP_TO_DATE);

  for (const char *version : {"2026.1-rc1", "4294967296.1", "2026.1."}) {
    repository.files.front() = MakeRepositoryOffer(version);
    ok1(!UpdateRepository::FindOffer(repository, "UNIX", ALLOWED_HOSTS,
                                     "2025.4"));
  }

  repository.files.front() = MakeRepositoryOffer();
  repository.files.front().uri = "https://xcsoar.org/" + std::string(300, 'x');
  ok1(!UpdateRepository::FindOffer(repository, "UNIX", ALLOWED_HOSTS,
                                   "2025.4"));
  ok1(!IsAllowedUrl("http://xcsoar.org/download", ALLOWED_HOSTS));
  ok1(!IsAllowedUrl("https://example.org/download", ALLOWED_HOSTS));

  repository.files.front() = MakeRepositoryOffer();
  ok1(!UpdateRepository::FindOffer(repository, "ANDROID", ALLOWED_HOSTS,
                                   "2025.4"));

  repository.files.emplace_back(MakeRepositoryOffer("2027.1"));
  const auto newest = UpdateRepository::FindOffer(repository, "UNIX",
                                                   ALLOWED_HOSTS, "2025.4");
  ok1(newest && newest->info);
  ok1(newest->info->version.equals("2027.1"));
}

int
main()
{
  plan_tests(76);
  TestSchedule();
  TestAutomaticCheckDefault();
  TestOwnedBackend();
  TestStateJSON();
  TestService();
  TestCachedResult();
  TestStartupCompletion();
  TestThrowingResultListener();
  TestBackendFailures();
  TestMultipleBackends();
  TestRepositoryOffer();
  return exit_status();
}
