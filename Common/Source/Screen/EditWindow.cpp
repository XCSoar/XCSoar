#include "Screen/EditWindow.hpp"

#ifdef PNA
#include "Appearance.hpp" // for GlobalModelType
#include "Asset.hpp" // for MODELTYPE_*
#endif

void
EditWidget::set(ContainerWindow &parent, int left, int top,
                unsigned width, unsigned height,
                bool multiline)
{
  DWORD style = WS_BORDER | WS_VISIBLE | WS_CHILD
    | ES_LEFT
    | WS_CLIPCHILDREN
    | WS_CLIPSIBLINGS;
  DWORD ex_style = 0;

  if (multiline)
    style |= ES_MULTILINE | WS_VSCROLL;
  else
    style |= ES_AUTOHSCROLL;

#ifdef PNA // VENTA3 FIX
  if (GlobalModelType == MODELTYPE_PNA_HP31X)
    ex_style |= WS_EX_CLIENTEDGE;
#endif

  Window::set(&parent, TEXT("EDIT"), TEXT("\0"),
              left, top, width, height, style, ex_style);
}
