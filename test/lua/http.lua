request = xcsoar.http.Request:new("https://download.xcsoar.org/repository")
response = request:perform()
print("status", response.status)

for name, value in pairs(response.headers) do
    print("header", name, ":", value)
end

print("body", response.body)
