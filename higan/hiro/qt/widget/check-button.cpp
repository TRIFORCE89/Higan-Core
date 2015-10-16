#if defined(Hiro_CheckButton)

namespace hiro {

auto pCheckButton::construct() -> void {
  qtWidget = qtCheckButton = new QtCheckButton(*this);
  qtCheckButton->setCheckable(true);
  qtCheckButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
  qtCheckButton->connect(qtCheckButton, SIGNAL(toggled(bool)), SLOT(onToggle(bool)));

  pWidget::construct();
  _setState();
}

auto pCheckButton::destruct() -> void {
  delete qtCheckButton;
  qtCheckButton = nullptr;
}

auto pCheckButton::minimumSize() const -> Size {
  auto size = pFont::size(qtWidget->font(), state().text);

  if(state().orientation == Orientation::Horizontal) {
    size.setWidth(size.width() + state().image.width());
    size.setHeight(max(state().image.height(), size.height()));
  }

  if(state().orientation == Orientation::Vertical) {
    size.setWidth(max(state().image.width(), size.width()));
    size.setHeight(size.height() + state().image.height());
  }

  return {size.width() + 20, size.height() + 12};
}

auto pCheckButton::setBordered(bool bordered) -> void {
  _setState();
}

auto pCheckButton::setChecked(bool checked) -> void {
  _setState();
}

auto pCheckButton::setImage(const Image& image) -> void {
  _setState();
}

auto pCheckButton::setOrientation(Orientation orientation) -> void {
  _setState();
}

auto pCheckButton::setText(const string& text) -> void {
  _setState();
}

auto pCheckButton::_setState() -> void {
  lock();
  qtCheckButton->setAutoRaise(!state().bordered);
  qtCheckButton->setChecked(state().checked);
  qtCheckButton->setIconSize(QSize(state().image.width(), state().image.height()));
  qtCheckButton->setIcon(CreateImage(state().image));
  qtCheckButton->setStyleSheet("text-align: top;");
  switch(state().orientation) {
  case Orientation::Horizontal: qtCheckButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon); break;
  case Orientation::Vertical:   qtCheckButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon); break;
  }
  qtCheckButton->setText(QString::fromUtf8(state().text));
  unlock();
}

auto QtCheckButton::onToggle(bool checked) -> void {
  p.state().checked = checked;
  if(!p.locked()) p.self().doToggle();
}

}

#endif
