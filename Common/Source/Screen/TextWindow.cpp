#include "Screen/TextWindow.hpp"

void
TextWidget::set(ContainerWindow &parent, unsigned left, unsigned top,
                unsigned width, unsigned height,
                bool center, bool notify, bool show,
                bool tabstop, bool border)
{
  Window::set(&parent, TEXT("STATIC"), TEXT(" "),
              left, top, width, height,
              center, notify, show, tabstop, border);
}
