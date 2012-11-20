#include <QBrush>
#include <QPainter>
#include <QPaintEvent>
#include <QPen>
#include <QScrollArea>
#include <QScrollBar>
#include <QSplitter>
#include <QTreeView>
#include <QVBoxLayout>

#include "color.h"
#include "colormanager.h"
#include "globalstate.h"
#include "palettemodel.h"

#include "palettewidget.h"

#define SWATCH_WIDTH 30
#define SWATCH_HEIGHT 15

/* SwatchWidget */

SwatchWidget::SwatchWidget(ColorManager *cm, QWidget *parent)
    : QWidget(parent), list_(&cm->colorList())
{
  picking_ = false;

  setSizeIncrement(SWATCH_WIDTH, SWATCH_HEIGHT);
  setMouseTracking(true);

  connect(cm, SIGNAL(listChanged()), this, SLOT(update()));
}

SwatchWidget::~SwatchWidget()
{
  
}

void SwatchWidget::setColorManager(ColorManager *cm)
{
  list_ = &cm->colorList();

  disconnect(this, SLOT(update()));
  connect(cm, SIGNAL(listChanged()), this, SLOT(update()));
}

QPoint SwatchWidget::itemPosition(int idx) const
{
  int cols = width() / SWATCH_WIDTH;

  return QPoint((idx % cols) * SWATCH_WIDTH, (idx / cols) * SWATCH_HEIGHT);
}

int SwatchWidget::itemIndex(const QPoint &point) const
{
  QPoint pos(point.x() / SWATCH_WIDTH, point.y() / SWATCH_HEIGHT);
  int cols = width() / SWATCH_WIDTH;
  if (pos.x() >= cols)
    return -1;

  int index = cols * pos.y() + pos.x();
  if (index < 0 || index >= list_->size())
    return -1;

  return index;
}

QSize SwatchWidget::minimumSizeHint() const
{
  return sizeHint();
}

QSize SwatchWidget::sizeHint() const
{
  int cols = width() / SWATCH_WIDTH;
  int rows = list_->size() / cols;
  
  if (list_->size() % cols > 0)
    rows += 1;

  return QSize(cols * SWATCH_WIDTH, rows * SWATCH_HEIGHT);
}

QPoint SwatchWidget::mapToTable(const QPoint &pos) const
{
  return QPoint(pos.x() / SWATCH_WIDTH, pos.y() / SWATCH_HEIGHT);
}

const Color* SwatchWidget::mapToElem(const QPoint &pos) const
{
  int cols = width() / SWATCH_WIDTH;

  if (pos.x() >= cols)
    return NULL;

  int index = cols * pos.y() + pos.x();
  if (index >= list_->size())
    return NULL;

  return (*list_)[index];
}

void SwatchWidget::mousePressEvent(QMouseEvent *event)
{
  event->accept();

  if (event->button() & Qt::LeftButton) {
    picking_ = true;

    mouseMoveEvent(event);
  }
}

void SwatchWidget::mouseMoveEvent(QMouseEvent *event)
{
  int index = itemIndex(event->pos());
  if (index != -1) {
    if (picking_)
      emit indexSelected(index);
    emit indexHovered(index);
  }
}

void SwatchWidget::mouseReleaseEvent(QMouseEvent *event)
{
  event->accept();

  if (event->button() & Qt::LeftButton && picking_) {
    picking_ = false;
  }
}

void SwatchWidget::paintEvent(QPaintEvent *event)
{
  QPoint tl = mapToTable(event->rect().topLeft());
  QPoint br = mapToTable(event->rect().bottomRight());

  const Color *currentColor = GlobalState::self()->color();

  QPainter p;
  p.begin(this);
  for (int i = tl.y(); i <= br.y(); ++i) {
    for (int j = tl.x(); j <= br.x(); ++j) {
      const Color *c = mapToElem(QPoint(j, i));
      if (c) {
        if (c == currentColor) {
          p.setPen(QPen());
          p.setBrush(Qt::black);
          p.drawRect(QRect(j * SWATCH_WIDTH, i * SWATCH_HEIGHT,
                         SWATCH_WIDTH, SWATCH_HEIGHT));

          p.setBrush(QBrush(c->color()));
          p.drawRect(QRect(j * SWATCH_WIDTH + 5, i * SWATCH_HEIGHT + 4,
                         SWATCH_WIDTH - 10, SWATCH_HEIGHT - 8));
        } else {
          p.setPen(QPen(c->color().darker()));
          p.setBrush(QBrush(c->color()));
          p.drawRect(QRect(j * SWATCH_WIDTH, i * SWATCH_HEIGHT,
                         SWATCH_WIDTH, SWATCH_HEIGHT));
        }
        
        
      }
    }
  }
  p.end();
}

/* PaletteListView */

PaletteListView::PaletteListView(QWidget *parent)
    : QTreeView(parent)
{
  setRootIsDecorated(false);
  setSelectionBehavior(QTreeView::SelectRows);
  setMouseTracking(true);
}

PaletteListView::~PaletteListView()
{

}

void PaletteListView::selectItem(int row)
{
  selectionModel()->select(
      QItemSelection(
          model()->index(row, 0),
          model()->index(row, model()->columnCount() - 1)),
      QItemSelectionModel::ClearAndSelect);
  scrollTo(model()->index(row, 0));
}

void PaletteListView::mouseMoveEvent(QMouseEvent *event)
{
  QTreeView::mouseMoveEvent(event);

  QModelIndex idx = indexAt(event->pos());
  if (idx.isValid())
    emit itemHovered(idx.row());
}

void PaletteListView::selectionChanged(const QItemSelection &selected,
                                       const QItemSelection &deselected)
{
  QTreeView::selectionChanged(selected, deselected);

  int row = selected.indexes()[0].row();
  emit itemSelected(row);
}

/* Palette */

Palette::Palette(QWidget *parent)
    : QFrame(parent)
{
  setFrameStyle(QFrame::Panel);

  hovered_ = NULL;
}

Palette::~Palette()
{

}

QSize Palette::sizeHint() const
{
  return QSize(10, 40);
}

QSize Palette::minimumSizeHint() const
{
  return sizeHint();
}

void Palette::setColor(const Color *c)
{
  hovered_ = c;
}
  
void Palette::paintEvent(QPaintEvent *event)
{
  QPainter p;
  
  p.begin(this);
  
  p.setPen(Qt::NoPen);
  const Color *selected = GlobalState::self()->color();
  if (selected) {
    p.setBrush(QBrush(selected->color()));
    p.drawRect(0, 0, width() / 2, height());
  }
  if (hovered_) {
    p.setBrush(QBrush(hovered_->color()));
    p.drawRect(width() / 2, 0, width() / 2, height());
  }

  p.end();

  QFrame::paintEvent(event);
}

/* PaletteWidget */

class MyScrollArea : public QScrollArea
{
 public:
  MyScrollArea(QWidget *parent) : QScrollArea(parent) { }
  ~MyScrollArea() { }

  QSize sizeHint() const {
    return QSize(8 * SWATCH_WIDTH + 25, 100);
  }

 private:
  void resizeEvent(QResizeEvent *event) {
    if (widget())
      widget()->resize(event->size());
  }
};

PaletteWidget::PaletteWidget(ColorManager *cm, QWidget *parent)
    : QWidget(parent)
{
  QVBoxLayout *rootLayout = new QVBoxLayout(this);

  QSplitter *splitter = new QSplitter(Qt::Vertical);

  swatchScrollArea_ = new MyScrollArea(splitter);
  swatchWidget_ = new SwatchWidget(cm);
  swatchScrollArea_->setWidgetResizable(true);
  swatchScrollArea_->setWidget(swatchWidget_);

  listWidget_ = new PaletteListView(splitter);
  model_ = new PaletteModel(cm, listWidget_);
  listWidget_->setModel(model_);

  palette_ = new Palette();
  
  rootLayout->addWidget(splitter);
  rootLayout->addWidget(palette_);

  connect(listWidget_, SIGNAL(itemSelected(int)), this, SLOT(itemSelected(int)));
  connect(listWidget_, SIGNAL(itemHovered(int)), this, SLOT(itemHovered(int)));
  connect(swatchWidget_, SIGNAL(indexSelected(int)), listWidget_, SLOT(selectItem(int)));
  connect(swatchWidget_, SIGNAL(indexHovered(int)), this, SLOT(itemHovered(int)));
  
  setLayout(rootLayout);

  setColorManager(cm);
}

PaletteWidget::~PaletteWidget()
{

}

void PaletteWidget::setColorManager(ColorManager *cm)
{
  swatchWidget_->setColorManager(cm);
  model_->setColorManager(cm);
  list_ = &cm->colorList();
}

void PaletteWidget::itemSelected(int row)
{
  if (list_) {
    emit colorSelected((*list_)[row]);
    QPoint pos = swatchWidget_->itemPosition(row);
    swatchScrollArea_->ensureVisible(pos.x(), pos.y());
    swatchWidget_->update();
    palette_->update();
  }
}

void PaletteWidget::itemHovered(int row)
{
  if (list_) {
    const Color *c = (*list_)[row];
    palette_->setColor(c);
    palette_->update();
  }
}