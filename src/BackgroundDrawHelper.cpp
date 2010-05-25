#include "BackgroundDrawHelper.hpp"
#include "Terrain/RasterTerrain.hpp"
#include "Terrain/TerrainRenderer.hpp"
#include "Components.hpp"
#include "SettingsUser.hpp"

BackgroundDrawHelper::BackgroundDrawHelper():
  m_rend(NULL)
{


}

BackgroundDrawHelper::~BackgroundDrawHelper()
{
  if (m_rend) {
    delete m_rend;
  }
}

void 
BackgroundDrawHelper::Draw(Canvas& canvas,
                           const RECT& rc,
                           const Projection& proj,
                           const SETTINGS_MAP& settings_map)
{
  if (!terrain.isTerrainLoaded())
    return;
  
  if (!m_rend) {
    m_rend = new TerrainRenderer(&terrain, rc);
  }
  m_rend->SetSettings(settings_map.TerrainRamp,
                      settings_map.TerrainContrast,
                      settings_map.TerrainBrightness);

  m_rend->Draw(canvas, proj, 
               Angle::degrees(fixed(45.0)), 
               Angle::degrees(fixed(45.0)));
}

