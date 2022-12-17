/****************************************************************************
 *   Copyright (c) 2020 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#include "PreCompiled.h"

#ifndef _PreComp_
# include <QPointer>
# include <QPainter>
# include <QDockWidget>
# include <QMdiArea>
# include <QTabBar>
# include <QTreeView>
# include <QHeaderView>
# include <QToolTip>
# include <QAction>
# include <QKeyEvent>
# include <QTextStream>
# include <QComboBox>
# include <QBoxLayout>
# include <QSpacerItem>
# include <QSplitter>
# include <QMenu>
# include <QScrollBar>
# include <QTimerEvent>
#endif

#if QT_VERSION >= 0x050000
# include <QWindow>
#endif

#include <QPainterPath>
#include <QPropertyAnimation>

#include <array>
#include <unordered_map>

#include <Base/Tools.h>
#include <Base/Console.h>
#include "BitmapFactory.h"
#include "Command.h"
#include "MainWindow.h"
#include "OverlayParams.h"
#include "View3DInventor.h"
#include "View3DInventorViewer.h"
#include "SplitView3DInventor.h"
#include "Application.h"
#include "Control.h"
#include "TaskView/TaskView.h"
#include "Clipping.h"
#include "ComboView.h"
#include "Tree.h"
#include "TreeParams.h"
#include <App/Application.h>
#include "propertyeditor/PropertyEditor.h"
#include "OverlayWidgets.h"
#include "NaviCube.h"

FC_LOG_LEVEL_INIT("Dock", true, true);

using namespace Gui;

static OverlayTabWidget *_LeftOverlay;
static OverlayTabWidget *_RightOverlay;
static OverlayTabWidget *_TopOverlay;
static OverlayTabWidget *_BottomOverlay;
static std::array<OverlayTabWidget*, 4> _Overlays;

static OverlayDragFrame *_DragFrame;
static QDockWidget *_DragFloating;
static QWidget *_Dragging;
static bool _ChangingOverlayMode;

static const int _MinimumOverlaySize = 30;

#define TITLE_BUTTON_COLOR "# c #202020"

static inline int widgetMinSize(const QWidget *widget, bool margin=false)
{
    return widget->fontMetrics().ascent()
        + widget->fontMetrics().descent() + (margin?4:0);
}

static QWidget *createTitleButton(QAction *action, int size)
{
    auto button = new OverlayToolButton(nullptr);
    button->setObjectName(action->data().toString());
    button->setDefaultAction(action);
    button->setAutoRaise(true);
    button->setContentsMargins(0,0,0,0);
    button->setFixedSize(size,size);
    return button;
}

static inline void setFocusView()
{
    auto view = getMainWindow()->activeWindow();
    if (!view)
        view = Application::Instance->activeView();
    if (view)
        view->setFocus();
}

// -----------------------------------------------------------

OverlayProxyWidget::OverlayProxyWidget(OverlayTabWidget *tabOverlay)
    :QWidget(tabOverlay->parentWidget()), owner(tabOverlay), _hintColor(QColor(50,50,50,150))
{
    dockArea = owner->getDockArea();
    timer.setSingleShot(true);
    connect(&timer, SIGNAL(timeout()), this, SLOT(onTimer()));
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
}

bool OverlayProxyWidget::isActivated() const
{
    return drawLine && isVisible();
}

int OverlayProxyWidget::hitTest(const QPoint &globalPt, bool delay)
{
    if (!isVisible() || !owner->count())
        return false;

    auto pt = mapFromGlobal(globalPt);

    QTabBar *tabbar = owner->tabBar();
    if (tabbar->isVisible() && tabbar->tabAt(pt)>=0) {
        ToolTip::showText(globalPt, QObject::tr("Press ESC to hide hint"), this);
        return 1;
    }

    int hit = 0;
    QRect rect = this->getRect();
    QSize s = this->size();
    int hintSize = OverlayParams::getDockOverlayHintTriggerSize();
    // if (owner->getState() == OverlayTabWidget::State_HintHidden)
    //     hintSize *= 2;
    switch(dockArea) {
    case Qt::LeftDockWidgetArea:
        hit = (pt.y() >= 0 && pt.y() <= s.height() && pt.x() > 0 && pt.x() < hintSize);
        break;
    case Qt::RightDockWidgetArea:
        hit = (pt.y() >= 0 && pt.y() <= s.height() && pt.x() < s.width() && pt.x() > -hintSize);
        break;
    case Qt::TopDockWidgetArea:
        hit = (pt.x() >= 0 && pt.x() <= s.width() && pt.y() > 0 && pt.y() < hintSize);
        break;
    case Qt::BottomDockWidgetArea:
        hit = (pt.x() >= 0 && pt.x() <= s.width() && pt.y() < s.height() && pt.y() > -hintSize);
        break;
    }
    if (rect.contains(pt)) {
        hit = 2;
        ToolTip::showText(globalPt, QObject::tr("Press ESC to hide hint"), this);
    } else if (drawLine)
        ToolTip::hideText();

    if (owner->getState() == OverlayTabWidget::State_HintHidden) {
        if (!hit)
            owner->setState(OverlayTabWidget::State_Normal);
        else {
            hit = 0;
            ToolTip::hideText();
        }
    }
    if (hit) {
        if (drawLine)
            timer.stop();
        else if (delay) {
            if (!timer.isActive())
                timer.start(OverlayParams::getDockOverlayHintDelay());
            return true;
        } else {
            timer.stop();
            owner->setState(OverlayTabWidget::State_Hint);
            drawLine = true;
            update();
        }
        if(owner->getState() != OverlayTabWidget::State_Hidden
                && hit > 1
                && OverlayParams::getDockOverlayActivateOnHover()) {
            if (owner->isVisible() && owner->tabBar()->isVisible()) {
                QSize size = owner->tabBar()->size();
                QPoint pt = owner->tabBar()->mapToGlobal(
                                QPoint(size.width(), size.height()));
                QPoint pos = QCursor::pos();
                switch(this->dockArea) {
                case Qt::LeftDockWidgetArea:
                case Qt::RightDockWidgetArea:
                    if (pos.y() < pt.y())
                        return false;
                    break;
                case Qt::TopDockWidgetArea:
                case Qt::BottomDockWidgetArea:
                    if (pos.x() < pt.x())
                        return false;
                    break;
                default:
                    break;
                }
            }
            owner->setState(OverlayTabWidget::State_Showing);
        }

    } else if (!drawLine)
        timer.stop();
    else if (delay) {
        if (!timer.isActive())
            timer.start(OverlayParams::getDockOverlayHintDelay());
    } else {
        timer.stop();
        owner->setState(OverlayTabWidget::State_Normal);
        drawLine = false;
        ToolTip::hideText();
        update();
    }
    return hit;
}

void OverlayProxyWidget::onTimer()
{
    hitTest(QCursor::pos(), false);
}

void OverlayProxyWidget::enterEvent(QEvent *)
{
    if(!owner->count())
        return;

    if (!drawLine) {
        if (!timer.isActive())
            timer.start(OverlayParams::getDockOverlayHintDelay());
    }
}

void OverlayProxyWidget::leaveEvent(QEvent *)
{
    // drawLine = false;
    // update();
}

void OverlayProxyWidget::hideEvent(QHideEvent *)
{
    drawLine = false;
}

void OverlayProxyWidget::onMousePress()
{
    if(!owner->count())
        return;

    if (owner->getState() == OverlayTabWidget::State_HintHidden)
        return;

    owner->setState(OverlayTabWidget::State_Showing);
}

QBrush OverlayProxyWidget::hintColor() const
{
    return _hintColor;
}

void OverlayProxyWidget::setHintColor(const QBrush &brush)
{
    _hintColor = brush;
}

QRect OverlayProxyWidget::getRect() const
{
    QRect rect = this->rect();
    if (owner->isVisible() && owner->tabBar()->isVisible()) {
        QSize size = owner->tabBar()->size();
        QPoint pt = owner->tabBar()->mapToGlobal(
                        QPoint(size.width(), size.height()));
        pt = this->mapFromGlobal(pt);
        switch(this->dockArea) {
            case Qt::LeftDockWidgetArea:
            case Qt::RightDockWidgetArea:
                rect.setTop(pt.y());
                break;
            case Qt::TopDockWidgetArea:
            case Qt::BottomDockWidgetArea:
                rect.setLeft(pt.x());
                break;
            default:
                break;
        }
    }
    switch(this->dockArea) {
        case Qt::LeftDockWidgetArea:
            if (int offset = OverlayParams::getDockOverlayHintLeftOffset())
                rect.moveTop(std::max(rect.top()+offset, rect.bottom()-10));
            if (int length = OverlayParams::getDockOverlayHintLeftLength())
                 rect.setHeight(std::min(length, rect.height()));
            break;
        case Qt::RightDockWidgetArea:
            if (int offset = OverlayParams::getDockOverlayHintRightOffset())
                rect.moveTop(std::max(rect.top()+offset, rect.bottom()-10));
            if (int length = OverlayParams::getDockOverlayHintRightLength())
                 rect.setHeight(std::min(length, rect.height()));
            break;
        case Qt::TopDockWidgetArea:
            if (int offset = OverlayParams::getDockOverlayHintTopOffset())
                rect.moveLeft(std::max(rect.left()+offset, rect.right()-10));
            if (int length = OverlayParams::getDockOverlayHintTopLength())
                 rect.setWidth(std::min(length, rect.width()));
            break;
        case Qt::BottomDockWidgetArea:
            if (int offset = OverlayParams::getDockOverlayHintBottomOffset())
                rect.moveLeft(std::max(rect.left()+offset, rect.right()-10));
            if (int length = OverlayParams::getDockOverlayHintBottomLength())
                 rect.setWidth(std::min(length, rect.width()));
            break;
        default:
            break;
    }
    return rect;
}

void OverlayProxyWidget::paintEvent(QPaintEvent *)
{
    if(!drawLine)
        return;
    QPainter painter(this);
    painter.setOpacity(_hintColor.color().alphaF());
    painter.setPen(Qt::transparent);
    painter.setBrush(_hintColor);

    QRect rect = this->getRect();
    painter.drawRect(rect);
}

OverlayToolButton::OverlayToolButton(QWidget *parent)
    :QToolButton(parent)
{
    setCursor(Qt::ArrowCursor);
}

// --------------------------------------------------------------------

OverlayTabWidget::OverlayTabWidget(QWidget *parent, Qt::DockWidgetArea pos)
    :QTabWidget(parent), dockArea(pos)
{
    // This is necessary to capture any focus lost from switching the tab,
    // otherwise the lost focus will leak to the parent, i.e. MdiArea, which may
    // cause unexpected Mdi sub window switching.
    // setFocusPolicy(Qt::StrongFocus);

    _imageScale = 0.0;

    splitter = new OverlaySplitter(this);

    _graphicsEffect = new OverlayGraphicsEffect(splitter);
    splitter->setGraphicsEffect(_graphicsEffect);

    _graphicsEffectTab = new OverlayGraphicsEffect(this);
    _graphicsEffectTab->setEnabled(false);
    tabBar()->setGraphicsEffect(_graphicsEffectTab);

    Command *cmdHide = nullptr;
    switch(pos) {
    case Qt::LeftDockWidgetArea:
        _LeftOverlay = this;
        setTabPosition(QTabWidget::West);
        splitter->setOrientation(Qt::Vertical);
        cmdHide = Application::Instance->commandManager().getCommandByName("Std_DockOverlayToggleLeft");
        break;
    case Qt::RightDockWidgetArea:
        _RightOverlay = this;
        setTabPosition(QTabWidget::East);
        splitter->setOrientation(Qt::Vertical);
        cmdHide = Application::Instance->commandManager().getCommandByName("Std_DockOverlayToggleRight");
        break;
    case Qt::TopDockWidgetArea:
        _TopOverlay = this;
        setTabPosition(QTabWidget::North);
        splitter->setOrientation(Qt::Horizontal);
        cmdHide = Application::Instance->commandManager().getCommandByName("Std_DockOverlayToggleTop");
        break;
    case Qt::BottomDockWidgetArea:
        _BottomOverlay = this;
        setTabPosition(QTabWidget::South);
        splitter->setOrientation(Qt::Horizontal);
        cmdHide = Application::Instance->commandManager().getCommandByName("Std_DockOverlayToggleBottom");
        break;
    default:
        break;
    }

    proxyWidget = new OverlayProxyWidget(this);
    proxyWidget->hide();
    _setOverlayMode(proxyWidget,1);

    setOverlayMode(true);
    hide();

    actTransparent.setCheckable(true);
    actTransparent.setData(QStringLiteral("OBTN Transparent"));
    actTransparent.setParent(this);
    addAction(&actTransparent);

    actAutoHide.setData(QStringLiteral("OBTN AutoHide"));

    actEditHide.setData(QStringLiteral("OBTN EditHide"));

    actEditShow.setData(QStringLiteral("OBTN EditShow"));

    actTaskShow.setData(QStringLiteral("OBTN TaskShow"));

    actNoAutoMode.setData(QStringLiteral("OBTN NoAutoMode"));

    actAutoMode.setData(QStringLiteral("OBTN AutoMode"));
    actAutoMode.setParent(this);
    autoModeMenu.hide();
    autoModeMenu.setToolTipsVisible(true);
    autoModeMenu.addAction(&actNoAutoMode);
    autoModeMenu.addAction(&actAutoHide);
    autoModeMenu.addAction(&actEditShow);
    autoModeMenu.addAction(&actEditHide);
    autoModeMenu.addAction(&actTaskShow);
    addAction(&actAutoMode);

    actOverlay.setData(QStringLiteral("OBTN Overlay"));
    actOverlay.setParent(this);
    addAction(&actOverlay);

    if (cmdHide)
        cmdHide->addTo(this);

    retranslate();
    refreshIcons();

    connect(tabBar(), SIGNAL(tabBarClicked(int)), this, SLOT(onCurrentChanged(int)));
    connect(tabBar(), SIGNAL(tabMoved(int,int)), this, SLOT(onTabMoved(int,int)));
    tabBar()->installEventFilter(this);

    timer.setSingleShot(true);
    connect(&timer, SIGNAL(timeout()), this, SLOT(setupLayout()));

    repaintTimer.setSingleShot(true);
    connect(&repaintTimer, SIGNAL(timeout()), this, SLOT(onRepaint()));

    _animator = new QPropertyAnimation(this, "animation", this);
    _animator->setStartValue(0.0);
    _animator->setEndValue(1.0);
    connect(_animator, SIGNAL(stateChanged(QAbstractAnimation::State, 
                                           QAbstractAnimation::State)),
            this, SLOT(onAnimationStateChanged()));
}

void OverlayTabWidget::refreshIcons()
{
    actOverlay.setIcon(BitmapFactory().pixmap("qss:overlay/overlay.svg"));
    actNoAutoMode.setIcon(BitmapFactory().pixmap("qss:overlay/mode.svg"));
    actTaskShow.setIcon(BitmapFactory().pixmap("qss:overlay/taskshow.svg"));
    actEditShow.setIcon(BitmapFactory().pixmap("qss:overlay/editshow.svg"));
    actEditHide.setIcon(BitmapFactory().pixmap("qss:overlay/edithide.svg"));
    actTransparent.setIcon(BitmapFactory().pixmap("qss:overlay/transparent.svg"));
    QPixmap pxAutoHide = BitmapFactory().pixmap("qss:overlay/autohide.svg");
    switch(dockArea) {
    case Qt::LeftDockWidgetArea:
        actAutoHide.setIcon(pxAutoHide);
        break;
    case Qt::RightDockWidgetArea:
        actAutoHide.setIcon(pxAutoHide.transformed(QTransform().scale(-1,1)));
        break;
    case Qt::TopDockWidgetArea:
        actAutoHide.setIcon(pxAutoHide.transformed(QTransform().rotate(90)));
        break;
    case Qt::BottomDockWidgetArea:
        actAutoHide.setIcon(pxAutoHide.transformed(QTransform().rotate(-90)));
        break;
    default:
        break;
    }
    syncAutoMode();
}

void OverlayTabWidget::onAnimationStateChanged()
{
    if (_animator->state() != QAbstractAnimation::Running) {
        setAnimation(0);
        if (_animator->startValue().toReal() == 0.0) {
            hide();
            OverlayManager::instance()->refresh();
        }
        if (_state == State_Showing)
            setState(State_Normal);
    }
}

void OverlayTabWidget::setAnimation(qreal t)
{
    if (t != _animation) {
        _animation = t;
        setupLayout();
    }
}

void OverlayTabWidget::startShow()
{
    if (isVisible() || _state > State_Normal)
        return;
    int duration = OverlayParams::getDockOverlayAnimationDuration();
    bool setmode = _state != State_Showing;
    if (duration) {
        _animator->setStartValue(1.0);
        _animator->setEndValue(0.0);
        _animator->setDuration(duration);
        _animator->setEasingCurve((QEasingCurve::Type)OverlayParams::getDockOverlayAnimationCurve());
        _animator->start();
    }
    else if (_state == State_Showing)
        setState(State_Normal);
    proxyWidget->hide();
    show();
    raise();
    if (setmode)
        setOverlayMode(overlayed);
}

void OverlayTabWidget::startHide()
{
    if (!isVisible()
            || _state > State_Normal
            || (_animator->state() == QAbstractAnimation::Running
                && _animator->startValue().toReal() == 0.0))
        return;
    int duration = OverlayParams::getDockOverlayAnimationDuration();
    if (!duration)
        hide();
    else {
        _animator->setStartValue(0.0);
        _animator->setEndValue(1.0);
        _animator->setDuration(duration);
        _animator->setEasingCurve((QEasingCurve::Type)OverlayParams::getDockOverlayAnimationCurve());
        _animator->start();
    }
}

bool OverlayTabWidget::event(QEvent *ev)
{
    switch(ev->type()) {
    case QEvent::MouseButtonRelease:
        if(mouseGrabber() == this) {
            releaseMouse();
            ev->accept();
            return true;
        }
        break;
    case QEvent::MouseMove:
    case QEvent::ContextMenu:
        if(QApplication::mouseButtons() == Qt::NoButton && mouseGrabber() == this) {
            releaseMouse();
            ev->accept();
            return true;
        }
        break;
    case QEvent::MouseButtonPress:
        ev->accept();
        return true;
    default:
        break;
    }
    return QTabWidget::event(ev);
}

int OverlayTabWidget::testAlpha(const QPoint &_pos, int radiusScale)
{
    if (!count() || (!isOverlayed() && !isTransparent()) || !isVisible())
        return -1;

    if (tabBar()->isVisible() && tabBar()->tabAt(tabBar()->mapFromGlobal(_pos))>=0)
        return -1;

    if (titleBar->isVisible() && titleBar->rect().contains(titleBar->mapFromGlobal(_pos)))
        return -1;

    if (!splitter->isVisible())
        return 0;

    auto pos = splitter->mapFromGlobal(_pos);
    QSize size = splitter->size();
    if (pos.x() < 0 || pos.y() < 0
            || pos.x() >= size.width()
            || pos.y() >= size.height())
    {
        if (this->rect().contains(this->mapFromGlobal(_pos)))
            return 0;
        return -1;
    }

    if (_image.isNull()) {
        auto pixmap = splitter->grab();
        _imageScale = pixmap.devicePixelRatio();
        _image = pixmap.toImage();
    }

    int res = qAlpha(_image.pixel(pos*_imageScale));
    int radius = OverlayParams::getDockOverlayAlphaRadius() * radiusScale;
    if (res || radius<=0 )
        return res;

    radius *= _imageScale;
    for (int i=-radius; i<radius; ++i) {
        for (int j=-radius; j<radius; ++j) {
            if (pos.x()+i < 0 || pos.y()+j < 0
                    || pos.x()+i >= size.width()
                    || pos.y()+j >= size.height())
                continue;
            res = qAlpha(_image.pixel(pos*_imageScale + QPoint(i,j)));
            if (res)
                return res;
        }
    }
    return 0;
}

void OverlayTabWidget::paintEvent(QPaintEvent *ev)
{
    Base::StateLocker guard(repainting);
    repaintTimer.stop();
    if (!_image.isNull())
        _image = QImage();
    QTabWidget::paintEvent(ev);
}

void OverlayTabWidget::onRepaint()
{
    Base::StateLocker guard(repainting);
    repaintTimer.stop();
    if (!_image.isNull())
        _image = QImage();
    splitter->repaint();
}

void OverlayTabWidget::scheduleRepaint()
{
    if(!repainting
            && isVisible() 
            && _graphicsEffect)
    {
        repaintTimer.start(100);
    }
}

QColor OverlayTabWidget::effectColor() const
{
    return _graphicsEffect->color();
}

void OverlayTabWidget::setEffectColor(const QColor &color)
{
    _graphicsEffect->setColor(color);
    _graphicsEffectTab->setColor(color);
}

int OverlayTabWidget::effectWidth() const
{
    return _graphicsEffect->size().width();
}

void OverlayTabWidget::setEffectWidth(int s)
{
    auto size = _graphicsEffect->size();
    size.setWidth(s);
    _graphicsEffect->setSize(size);
    _graphicsEffectTab->setSize(size);
}

int OverlayTabWidget::effectHeight() const
{
    return _graphicsEffect->size().height();
}

void OverlayTabWidget::setEffectHeight(int s)
{
    auto size = _graphicsEffect->size();
    size.setHeight(s);
    _graphicsEffect->setSize(size);
    _graphicsEffectTab->setSize(size);
}

qreal OverlayTabWidget::effectOffsetX() const
{
    return _graphicsEffect->offset().x();
}

void OverlayTabWidget::setEffectOffsetX(qreal d)
{
    auto offset = _graphicsEffect->offset();
    offset.setX(d);
    _graphicsEffect->setOffset(offset);
    _graphicsEffectTab->setOffset(offset);
}

qreal OverlayTabWidget::effectOffsetY() const
{
    return _graphicsEffect->offset().y();
}

void OverlayTabWidget::setEffectOffsetY(qreal d)
{
    auto offset = _graphicsEffect->offset();
    offset.setY(d);
    _graphicsEffect->setOffset(offset);
    _graphicsEffectTab->setOffset(offset);
}

qreal OverlayTabWidget::effectBlurRadius() const
{
    return _graphicsEffect->blurRadius();
}

void OverlayTabWidget::setEffectBlurRadius(qreal r)
{
    _graphicsEffect->setBlurRadius(r);
    _graphicsEffectTab->setBlurRadius(r);
}

bool OverlayTabWidget::effectEnabled() const
{
    return _effectEnabled;
}

void OverlayTabWidget::setEffectEnabled(bool enable)
{
    _effectEnabled = enable;
}

bool OverlayTabWidget::eventFilter(QObject *o, QEvent *ev)
{
    if(ev->type() == QEvent::Resize && o == tabBar()) {
        if (_state <= State_Normal)
            timer.start(10);
    }
    return QTabWidget::eventFilter(o, ev);
}

void OverlayTabWidget::restore(ParameterGrp::handle handle)
{
    if (!handle) {
        hGrp = handle;
        return;
    }
    if (!parentWidget())
        return;
    std::string widgets = handle->GetASCII("Widgets","");
    for(auto &name : QString::fromUtf8(widgets.c_str()).split(QLatin1Char(','))) {
        if(name.isEmpty())
            continue;
        OverlayManager::instance()->registerDockWidget(name, this);
        auto dock = getMainWindow()->findChild<QDockWidget*>(name);
        if(dock)
            addWidget(dock, dock->windowTitle());
    }
    int width = handle->GetInt("Width", 0);
    int height = handle->GetInt("Height", 0);
    int offset1 = handle->GetInt("Offset1", 0);
    int offset2 = handle->GetInt("Offset3", 0);
    setOffset(QSize(offset1,offset2));
    setSizeDelta(handle->GetInt("Offset2", 0));
    if(width && height) {
        QRect rect(0, 0, width, height);
        switch(dockArea) {
        case Qt::RightDockWidgetArea:
            rect.moveRight(parentWidget()->size().width());
            break;
        case Qt::BottomDockWidgetArea:
            rect.moveBottom(parentWidget()->size().height());
            break;
        default:
            break;
        }
        setRect(rect);
    }
    if (handle->GetBool("AutoHide", false))
        setAutoMode(AutoHide);
    else if (handle->GetBool("EditHide", false))
        setAutoMode(EditHide);
    else if (handle->GetBool("EditShow", false))
        setAutoMode(EditShow);
    else if (handle->GetBool("TaskShow", false))
        setAutoMode(TaskShow);
    else
        setAutoMode(NoAutoMode);

    setTransparent(handle->GetBool("Transparent", false));

    _sizemap.clear();
    std::string savedSizes = handle->GetASCII("Sizes","");
    QList<int> sizes;
    int idx = 0;
    for(auto &size : QString::fromUtf8(savedSizes.c_str()).split(QLatin1Char(','))) {
        sizes.append(size.toInt());
        _sizemap[dockWidget(idx++)] = sizes.back();
    }

    FC_LOG("restore " << objectName().toUtf8().constData() << " " << savedSizes);

    getSplitter()->setSizes(sizes);
    hGrp = handle;
}

void OverlayTabWidget::saveTabs()
{
    if(!hGrp)
        return;

    std::ostringstream os, os2;
    _sizemap.clear();
    auto sizes = splitter->sizes();
    bool first = true;
    for(int i=0,c=splitter->count(); i<c; ++i) {
        auto dock = dockWidget(i);
        if (!dock)
            continue;
        if(dock->objectName().size()) {
            os << dock->objectName().toUtf8().constData() << ",";
            if (first)
                first = false;
            else
                os2 << ",";
            os2 << sizes[i];
        }
        _sizemap[dock] = sizes[i];
    }
    Base::StateLocker lock(_saving);
    hGrp->SetASCII("Widgets", os.str().c_str());
    hGrp->SetASCII("Sizes", os2.str().c_str());
    FC_LOG("save " << objectName().toUtf8().constData() << " " << os2.str());
}

void OverlayTabWidget::onTabMoved(int from, int to)
{
    QWidget *w = splitter->widget(from);
    splitter->insertWidget(to,w);
    saveTabs();
}

void OverlayTabWidget::setTitleBar(QWidget *w)
{
    titleBar = w;
}

void OverlayTabWidget::changeEvent(QEvent *e)
{
    QTabWidget::changeEvent(e);
    if (e->type() == QEvent::LanguageChange)
        retranslate();
}

void OverlayTabWidget::retranslate()
{
    actTransparent.setToolTip(tr("Toggle transparent mode"));
    actNoAutoMode.setText(tr("None"));
    actNoAutoMode.setToolTip(tr("Turn off auto hide/show"));
    actAutoHide.setText(tr("Auto hide"));
    actAutoHide.setToolTip(tr("Auto hide docked widgets on leave"));
    actEditHide.setText(tr("Hide on edit"));
    actEditHide.setToolTip(tr("Auto hide docked widgets on editing"));
    actEditShow.setText(tr("Show on edit"));
    actEditShow.setToolTip(tr("Auto show docked widgets on editing"));
    actTaskShow.setText(tr("Auto task"));
    actTaskShow.setToolTip(tr("Auto show task view for any current task, and hide the view when there is no task."));
    actIncrease.setToolTip(tr("Increase window size, either width or height depending on the docking site.\n"
                              "Hold CTRL key while pressing the button to change size in the other dimension.\n"
                              "Hold SHIFT key while pressing the button to move the window.\n"
                              "Hold CTRL + SHIFT key to move the window in the other direction."));
    actDecrease.setToolTip(tr("Decrease window size, either width or height depending on the docking site.\n"
                              "Hold CTRL key while pressing to change size in the other dimension.\n"
                              "Hold SHIFT key while pressing the button to move the window.\n"
                              "Hold CTRL + SHIFT key to move the window in the other direction."));
    actOverlay.setToolTip(tr("Toggle overlay"));
    syncAutoMode();
}

void OverlayTabWidget::syncAutoMode()
{
    QAction *action = nullptr;
    switch(autoMode) {
    case AutoHide:
        action = &actAutoHide;
        break;
    case EditShow:
        action = &actEditShow;
        break;
    case TaskShow:
        action = &actTaskShow;
        break;
    case EditHide:
        action = &actEditHide;
        break;
    default:
        action = &actNoAutoMode;
        break;
    }
    actAutoMode.setIcon(action->icon());
    if (action == &actNoAutoMode)
        actAutoMode.setToolTip(tr("Select auto show/hide mode"));
    else
        actAutoMode.setToolTip(action->toolTip());
}

void OverlayTabWidget::onAction(QAction *action)
{
    if (action == &actAutoMode) {
        action = autoModeMenu.exec(QCursor::pos());
        if (action == &actNoAutoMode)
            setAutoMode(NoAutoMode);
        else if (action == &actAutoHide)
            setAutoMode(AutoHide);
        else if (action == &actEditShow)
            setAutoMode(EditShow);
        else if (action == &actTaskShow)
            setAutoMode(TaskShow);
        else if (action == &actEditHide)
            setAutoMode(EditHide);
        return;
    }
    else if(action == &actIncrease)
        changeSize(5);
    else if(action == &actDecrease)
        changeSize(-5);
    else if(action == &actOverlay) {
        OverlayManager::instance()->setOverlayMode(OverlayManager::ToggleActive);
        return;
    } else if(action == &actTransparent) {
        if(hGrp) {
            Base::StateLocker lock(_saving);
            hGrp->SetBool("Transparent", actTransparent.isChecked());
        }
    }
    OverlayManager::instance()->refresh(this);
}

void OverlayTabWidget::setState(State state)
{
    if (_state == state)
        return;
    switch(state) {
    case State_Normal:
        if (_state == State_Hidden) {
            // Only unhide through State_Showing, not State_Normal
            return;
        }
        else if (_state == State_Showing) {
            _state = state;
            return;
        }
        // fall through
    case State_Showing:
        _state = state;
        hide();
        if (dockArea == Qt::RightDockWidgetArea)
            setTabPosition(East);
        else if (dockArea == Qt::BottomDockWidgetArea)
            setTabPosition(South);
        if (this->count() == 1)
            tabBar()->hide();
        _graphicsEffectTab->setEnabled(false);
        titleBar->show();
        splitter->show();
        if (state == State_Showing)
            OverlayManager::instance()->refresh(this);
        break;
    case State_Hint:
        if (_state == State_HintHidden || _state == State_Hidden)
            break;
        _state = state;
        if (this->count() && OverlayParams::getDockOverlayHintTabBar()) {
            tabBar()->setToolTip(proxyWidget->toolTip());
            tabBar()->show();
            titleBar->hide();
            splitter->hide();
            _graphicsEffectTab->setEnabled(true);
            show();
            raise();
            proxyWidget->raise();
            if (dockArea == Qt::RightDockWidgetArea)
                setTabPosition(West);
            else if (dockArea == Qt::BottomDockWidgetArea)
                setTabPosition(North);
            OverlayManager::instance()->refresh(this);
        }
        break;
    case State_HintHidden:
        if (_state != State_Hidden)
            _state = state;
        proxyWidget->hide();
        hide();
        _graphicsEffectTab->setEnabled(true);
        break;
    case State_Hidden:
        startHide();
        _state = state;
        break;
    default:
        break;
    }
}

bool OverlayTabWidget::checkAutoHide() const
{
    if(autoMode == AutoHide)
        return true;

    if(OverlayParams::getDockOverlayAutoView()) {
        auto view = getMainWindow()->activeWindow();
        if (!view) return true;
        if(!view->onHasMsg("CanPan")
                && view->parentWidget()
                && view->parentWidget()->isMaximized())
            return true;
    }

    if(autoMode == EditShow) {
        return !Application::Instance->editDocument() 
            && (!Control().taskPanel() || Control().taskPanel()->isEmpty(false));
    }

    if(autoMode == EditHide && Application::Instance->editDocument())
        return true;

    return false;
}

static inline OverlayTabWidget *findTabWidget(QWidget *widget=nullptr, bool filterDialog=false)
{
    if(!widget)
        widget = qApp->focusWidget();
    for(auto w=widget; w; w=w->parentWidget()) {
        auto tabWidget = qobject_cast<OverlayTabWidget*>(w);
        if(tabWidget) 
            return tabWidget;
        auto proxy = qobject_cast<OverlayProxyWidget*>(w);
        if(proxy)
            return proxy->getOwner();
        if(filterDialog && w->windowType() != Qt::Widget)
            break;
    }
    return nullptr;
}

void OverlayTabWidget::leaveEvent(QEvent*)
{
    if (titleBar && QWidget::mouseGrabber() == titleBar)
        return;
    OverlayManager::instance()->refresh();
}

void OverlayTabWidget::enterEvent(QEvent*)
{
    revealTime = QTime();
    OverlayManager::instance()->refresh();
}

void OverlayTabWidget::setRevealTime(const QTime &time)
{
    revealTime = time;
}

class OverlayStyleSheet: public ParameterGrp::ObserverType {
public:

    OverlayStyleSheet() {
        handle = App::GetApplication().GetParameterGroupByPath(
                "User parameter:BaseApp/Preferences/MainWindow");
        update();
        handle->Attach(this);
    }

    static OverlayStyleSheet *instance() {
        static OverlayStyleSheet *inst;
        if(!inst)
            inst = new OverlayStyleSheet;
        return inst;
    }

    void OnChange(Base::Subject<const char*> &, const char* sReason) {
        if(!sReason)
            return;
        if(strcmp(sReason, "StyleSheet")==0
                || strcmp(sReason, "OverlayActiveStyleSheet")==0)
        {
            OverlayManager::instance()->refresh(nullptr, true);
        }
    }

    void update() {
        QString mainstyle = QString::fromUtf8(handle->GetASCII("StyleSheet").c_str());

        QString prefix;
       
        if(!mainstyle.isEmpty()) {
            int dark = mainstyle.indexOf(QStringLiteral("dark"),0,Qt::CaseInsensitive);
            prefix = QStringLiteral("overlay:%1").arg(
                    dark<0 ? QStringLiteral("Light") : QStringLiteral("Dark"));
        }

        QString name;
        name = QString::fromUtf8(handle->GetASCII("OverlayActiveStyleSheet").c_str());
        if(name.isEmpty() && !prefix.isEmpty())
            name = prefix + QStringLiteral(".qss");
        else if (!QFile::exists(name))
            name = QStringLiteral("overlay:%1").arg(name);
        activeStyleSheet.clear();
        if(QFile::exists(name)) {
            QFile f(name);
            if(f.open(QFile::ReadOnly)) {
                QTextStream str(&f);
                activeStyleSheet = str.readAll();
            }
        }
        if(activeStyleSheet.isEmpty()) {
            static QString _default = QStringLiteral(
                "* {alternate-background-color: rgba(250,250,250,120)}"

                "QComboBox, QComboBox:editable, QComboBox:!editable, QLineEdit,"
                "QTextEdit, QPlainTextEdit, QAbstractSpinBox, QDateEdit, QDateTimeEdit,"
                "Gui--PropertyEditor--PropertyEditor QLabel "
                    "{background : palette(base);}"

                "QScrollBar { background: rgba(0,0,0,10);}"
                "QTabWidget::pane { background-color: transparent; border: transparent }"
                "Gui--OverlayTabWidget { qproperty-effectColor: rgba(0,0,0,0) }"
                "Gui--OverlayTabWidget::pane { background-color: rgba(250,250,250,80) }"

                "QTabBar {border : none;}"
                "QTabBar::tab {color: palette(text);"
                              "background-color: rgba(100,100,100,50);"
                              "padding: 5px}"
                "QTabBar::tab:selected {background-color: rgba(250,250,250,80);}"
                "QTabBar::tab:hover {background-color: rgba(250,250,250,200);}"

                "QHeaderView { background:transparent }"
                "QHeaderView::section {color: palette(text);"
                                      "background-color: rgba(250,250,250,50);"
                                      "border: 1px solid palette(dark);"
                                      "padding: 2px}"

                "QTreeView, QListView, QTableView {"
                            "background: rgb(250,250,250);"
                            "border: transparent;"
                            "selection-background-color: rgba(94, 144, 250, 0.7);}"
                "QListView::item:selected, QTreeView::item:selected {"
                            "background-color: rgba(94, 144, 250, 0.7);}"

                "Gui--PropertyEditor--PropertyEditor {"
                            "border: 1px solid palette(dark);"
                            "qproperty-groupTextColor: rgb(100, 100, 100);"
                            "qproperty-groupBackground: rgba(180, 180, 180, 0.7);}"

                "QToolTip {background-color: rgba(250,250,250,180);}"

                "Gui--TreeWidget QHeaderView:section {"
                            "height: 0px;"
                            "background-color: transparent;"
                            "padding: 0px;"
                            "border: transparent;}"

                "Gui--CallTipsList::item { background-color: rgba(200,200,200,200);}"
                "Gui--CallTipsList::item::selected { background-color: palette(highlight);}"

                "QPushButton { background: rgba(250,250,250,80);padding: 2px 4px;}"
                "QPushButton::hover { background: rgba(250,250,250,200);}"
                "QPushButton::focus { background: rgba(250,250,250,255);}"
                "QPushButton::pressed { background-color: #5e90fa;"
                                           "border: 1px inset palette(dark) }"
                "QPushButton::checked { background: rgba(100,100,100,100);"
                                           "border: 1px inset palette(dark) }"
                "QPushButton::checked:hover { background: rgba(150,150,150,200);"
                                                 "border: 1px inset palette(dark) }"
                "Gui--OverlayToolButton { background: transparent; padding: 0px; border: none }"
                "Gui--OverlayTitleBar,"
                "Gui--OverlaySplitterHandle { background-color: rgba(200, 200, 200, 150); }"
                "QWidget#ClippingScrollAreaContents, "
                "QScrollArea#ClippingScrollArea { border: none; background-color: #a0e6e6e6; }"
                "Gui--PropertyEditor--PropertyEditor > QWidget > QPushButton {text-align:left;padding-left:2px;}"
            );
            activeStyleSheet = _default;
        }
    }

    ParameterGrp::handle handle;
    QString activeStyleSheet;
    bool hideTab = false;
};

void OverlayTabWidget::_setOverlayMode(QWidget *widget, int enable)
{
    if(!widget)
        return;

#if QT_VERSION>QT_VERSION_CHECK(5,12,2) && QT_VERSION < QT_VERSION_CHECK(5,12,6)
    // Work around Qt bug https://bugreports.qt.io/browse/QTBUG-77006
    widget->setStyleSheet(OverlayStyleSheet::instance()->activeStyleSheet);
#endif

    if (qobject_cast<QScrollBar*>(widget)) {
        auto parent = widget->parentWidget();
        if (parent) {
            parent = parent->parentWidget();
            if (qobject_cast<PropertyEditor::PropertyEditor*>(parent)) {
                auto scrollArea = static_cast<QAbstractScrollArea*>(parent);
                if (scrollArea->verticalScrollBar() == widget) {
                    if (!OverlayParams::getDockOverlayHidePropertyViewScrollBar() || enable==0)
                        widget->setStyleSheet(QString());
                    else {
                        static QString _style = QStringLiteral("*{width:0}");
                        widget->setStyleSheet(_style);
                    }
                }
            }
            auto treeView = qobject_cast<TreeWidget*>(parent);
            if (treeView) {
                auto scrollArea = static_cast<QAbstractScrollArea*>(parent);
                if (scrollArea->verticalScrollBar() == widget) {
                    if (!TreeParams::getHideScrollBar() || enable==0)
                        widget->setStyleSheet(QString());
                    else {
                        static QString _style = QStringLiteral("*{width:0}");
                        widget->setStyleSheet(_style);
                    }
                }
            }

            if (treeView) {
                auto header = treeView->header();
                if (!TreeParams::getHideHeaderView() || enable==0)
                    header->setStyleSheet(QString());
                else {
                    static QString _style = QStringLiteral(
                            "QHeaderView:section {"
                              "height: 0px;"
                              "background-color: transparent;"
                              "padding: 0px;"
                              "border: transparent;}");
                    header->setStyleSheet(_style);
                }
            }
        }
    }

    auto tabbar = qobject_cast<QTabBar*>(widget);
    if(tabbar) {
        // Stylesheet QTabWidget::pane make the following two calls unnecessary
        //
        // tabbar->setDrawBase(enable>0);
        // tabbar->setDocumentMode(enable!=0);

        if(!tabbar->autoHide() || tabbar->count()>1) {
            if(!OverlayStyleSheet::instance()->hideTab)
                tabbar->setVisible(true);
            else
                tabbar->setVisible(enable==0 || (enable<0 && tabbar->count()>1));
            return;
        }
    }

    if (!qobject_cast<QScrollArea*>(widget)
            || !qobject_cast<Dialog::Clipping*>(widget->parentWidget())) {
        if(enable!=0) {
            widget->setWindowFlags(widget->windowFlags() | Qt::FramelessWindowHint);
        } else {
            widget->setWindowFlags(widget->windowFlags() & ~Qt::FramelessWindowHint);
        }
        widget->setAttribute(Qt::WA_NoSystemBackground, enable!=0);
        widget->setAttribute(Qt::WA_TranslucentBackground, enable!=0);
    }
}

void OverlayTabWidget::setOverlayMode(QWidget *widget, int enable)
{
    if(!widget || (qobject_cast<QDialog*>(widget)
                        && !qobject_cast<Dialog::Clipping*>(widget))
               || qobject_cast<TaskView::TaskBox*>(widget))
        return;

    if(widget != tabBar()) {
        if(OverlayParams::getDockOverlayAutoMouseThrough() && enable == -1) {
            widget->setMouseTracking(true);
        }
    }

    _setOverlayMode(widget, enable);

    if(qobject_cast<QComboBox*>(widget)) {
        // do not set child QAbstractItemView of QComboBox, otherwise the drop down box
        // won't be shown
        return;
    }
    for(auto child : widget->children())
        setOverlayMode(qobject_cast<QWidget*>(child), enable);
}

void OverlayTabWidget::setTransparent(bool enable)
{
    if(actTransparent.isChecked() == enable)
        return;
    if(hGrp) {
        Base::StateLocker lock(_saving);
        hGrp->SetBool("Transparent", enable);
    }
    actTransparent.setChecked(enable);
    OverlayManager::instance()->refresh(this);
}

bool OverlayTabWidget::isTransparent() const
{
    if (!actTransparent.isChecked())
        return false;
    if(OverlayParams::getDockOverlayAutoView()) {
        auto view = getMainWindow()->activeWindow();
        if (!view) return false;
        if(!view->onHasMsg("CanPan")
                && view->parentWidget()
                && view->parentWidget()->isMaximized())
            return false;
    }
    return true;
}

bool OverlayTabWidget::isOverlayed(int checkTransparentState) const
{
    if (checkTransparentState && currentTransparent != isTransparent())
        return checkTransparentState > 0;
    return overlayed;
}

void OverlayTabWidget::setAutoMode(OverlayAutoMode mode)
{
    if (autoMode == mode)
        return;
    autoMode = mode;

    if (hGrp) {
        bool autohide = false, editshow = false, edithide = false, taskshow = false;
        switch (mode) {
        case AutoHide:
            autohide = true;
            break;
        case EditShow:
            editshow = true;
            break;
        case EditHide:
            edithide = true;
            break;
        case TaskShow:
            taskshow = true;
            break;
        default:
            break;
        }
        Base::StateLocker lock(_saving);
        hGrp->SetBool("AutoHide", autohide);
        hGrp->SetBool("EditShow", editshow);
        hGrp->SetBool("EditHide", edithide);
        hGrp->SetBool("TaskShow", taskshow);
    }
    syncAutoMode();
    OverlayManager::instance()->refresh(this);
}

QDockWidget *OverlayTabWidget::currentDockWidget() const
{
    int index = -1;
    for(int size : splitter->sizes()) {
        ++index;
        if(size>0)
            return dockWidget(index);
    }
    return dockWidget(currentIndex());
}

QDockWidget *OverlayTabWidget::dockWidget(int index) const
{
    if(index < 0 || index >= splitter->count())
        return nullptr;
    return qobject_cast<QDockWidget*>(splitter->widget(index));
}

void OverlayTabWidget::updateSplitterHandles()
{
    if (overlayed || _state > State_Normal)
        return;
    for (int i=0, c=splitter->count(); i<c; ++i) {
        auto handle = qobject_cast<OverlaySplitterHandle*>(splitter->handle(i));
        if (handle)
            handle->showTitle(true);
    }
}

bool OverlayTabWidget::onEscape()
{
    if (getState() == OverlayTabWidget::State_Hint
            || getState() == OverlayTabWidget::State_Hidden) {
        setState(OverlayTabWidget::State_HintHidden);
        return true;
    }
    if (!isVisible())
        return false;
    if (titleBar->isVisible() && titleBar->underMouse()) {
        titleBar->hide();
        return true;
    }
    for (int i=0, c=splitter->count(); i<c; ++i) {
        auto handle = qobject_cast<OverlaySplitterHandle*>(splitter->handle(i));
        if (handle->isVisible() && handle->underMouse()) {
            handle->showTitle(false);
            return true;
        }
    }
    return false;
}

void OverlayTabWidget::setOverlayMode(bool enable)
{
    overlayed = enable;

    if(!isVisible() || !count())
        return;

    Base::StateLocker guard(_ChangingOverlayMode);

    touched = false;

    if (_state <= State_Normal) {
        titleBar->setVisible(!enable || OverlayManager::instance()->isMouseTransparent());
        for (int i=0, c=splitter->count(); i<c; ++i) {
            auto handle = qobject_cast<OverlaySplitterHandle*>(splitter->handle(i));
            if (handle)
                handle->showTitle(!enable);
        }
    }

    QString stylesheet;
    stylesheet = OverlayStyleSheet::instance()->activeStyleSheet;
    currentTransparent = isTransparent();

    int mode;
    if(!enable && isTransparent()) {
        mode = -1;
    } else if (enable && !isTransparent() && (autoMode == EditShow || autoMode == AutoHide)) {
        mode = 0;
    } else {
        mode = enable?1:0;
    }

    proxyWidget->setStyleSheet(stylesheet);
    this->setStyleSheet(stylesheet);
    setOverlayMode(this, mode);

    _graphicsEffect->setEnabled(effectEnabled() && (enable || isTransparent()));

    if (_state == State_Hint && OverlayParams::getDockOverlayHintTabBar()) {
        tabBar()->setToolTip(proxyWidget->toolTip());
        tabBar()->show();
    } else if (OverlayParams::getDockOverlayHideTabBar() || count()==1) {
        tabBar()->hide();
    } else {
        tabBar()->setToolTip(QString());
        tabBar()->setVisible(!enable || !OverlayStyleSheet::instance()->hideTab);
    }

    setRect(rectOverlay);
}

const QRect &OverlayTabWidget::getRect()
{
    return rectOverlay;
}

bool OverlayTabWidget::getAutoHideRect(QRect &rect) const
{
    rect = rectOverlay;
    int hintWidth = OverlayParams::getDockOverlayHintSize();
    switch(dockArea) {
    case Qt::LeftDockWidgetArea:
    case Qt::RightDockWidgetArea:
        if (_TopOverlay->isVisible() && _TopOverlay->_state <= State_Normal)
            rect.setTop(std::max(rect.top(), _TopOverlay->rectOverlay.bottom()));
        if (dockArea == Qt::RightDockWidgetArea)
            rect.setLeft(rect.left() + std::max(rect.width()-hintWidth,0));
        else
            rect.setRight(rect.right() - std::max(rect.width()-hintWidth,0));
        break;
    case Qt::TopDockWidgetArea:
    case Qt::BottomDockWidgetArea:
        if (_LeftOverlay->isVisible() && _LeftOverlay->_state <= State_Normal)
            rect.setLeft(std::max(rect.left(),_LeftOverlay->rectOverlay.right()));
        if (dockArea == Qt::TopDockWidgetArea)
            rect.setBottom(rect.bottom() - std::max(rect.height()-hintWidth,0));
        else {
            rect.setTop(rect.top() + std::max(rect.height()-hintWidth,0));
            if (_RightOverlay->isVisible() && _RightOverlay->_state <= State_Normal) {
                QPoint offset = getMainWindow()->getMdiArea()->pos();
                rect.setRight(std::min(rect.right(), _RightOverlay->x()-offset.x()));
            }
        }
        break;
    default:
        break;
    }
    return _state != State_Showing && overlayed && checkAutoHide();
}

void OverlayTabWidget::setOffset(const QSize &ofs)
{
    if(offset != ofs) {
        offset = ofs;
        if(hGrp) {
            Base::StateLocker lock(_saving);
            hGrp->SetInt("Offset1", ofs.width());
            hGrp->SetInt("Offset3", ofs.height());
        }
    }
}

void OverlayTabWidget::setSizeDelta(int delta)
{
    if(sizeDelta != delta) {
        if(hGrp) {
            Base::StateLocker lock(_saving);
            hGrp->SetInt("Offset2", delta);
        }
        sizeDelta = delta;
    }
}

void OverlayTabWidget::setRect(QRect rect)
{
    if(busy || !parentWidget() || !getMainWindow() || !getMainWindow()->getMdiArea())
        return;

    if (rect.width() == 0)
        rect.setWidth(_MinimumOverlaySize*3);
    if (rect.height() == 0)
        rect.setHeight(_MinimumOverlaySize*3);

    switch(dockArea) {
    case Qt::LeftDockWidgetArea:
        rect.moveLeft(0);
        if (rect.width() < _MinimumOverlaySize)
            rect.setWidth(_MinimumOverlaySize);
        break;
    case Qt::RightDockWidgetArea:
        if (rect.width() < _MinimumOverlaySize)
            rect.setLeft(rect.right()-_MinimumOverlaySize);
        break;
    case Qt::TopDockWidgetArea:
        rect.moveTop(0);
        if (rect.height() < _MinimumOverlaySize)
            rect.setHeight(_MinimumOverlaySize);
        break;
    case Qt::BottomDockWidgetArea:
        if (rect.height() < _MinimumOverlaySize)
            rect.setTop(rect.bottom()-_MinimumOverlaySize);
        break;
    default:
        break;
    }

    if(hGrp && rect.size() != rectOverlay.size()) {
        Base::StateLocker lock(_saving);
        hGrp->SetInt("Width", rect.width());
        hGrp->SetInt("Height", rect.height());
    }
    rectOverlay = rect;

    QPoint offset = getMainWindow()->getMdiArea()->pos();

    if(getAutoHideRect(rect) || _state == State_Hint || _state == State_Hidden) {
        QRect rectHint = rect;
        if (_state != State_Hint && _state != State_Hidden)
            startHide();
        else if (count() && OverlayParams::getDockOverlayHintTabBar()) {
            switch(dockArea) {
            case Qt::LeftDockWidgetArea: 
            case Qt::RightDockWidgetArea: 
                if (dockArea == Qt::LeftDockWidgetArea)
                    rect.setWidth(tabBar()->width());
                else
                    rect.setLeft(rect.left() + rect.width() - tabBar()->width());
                rect.setHeight(std::max(rect.height(), 
                            tabBar()->y() + tabBar()->sizeHint().height() + 5));
                break;
            case Qt::BottomDockWidgetArea: 
            case Qt::TopDockWidgetArea: 
                if (dockArea == Qt::TopDockWidgetArea)
                    rect.setHeight(tabBar()->height());
                else
                    rect.setTop(rect.top() + rect.height() - tabBar()->height());
                rect.setWidth(std::max(rect.width(),
                            tabBar()->x() + tabBar()->sizeHint().width() + 5));
                break;
            default:
                break;
            }

            setGeometry(rect.translated(offset));
        }
        proxyWidget->setGeometry(rectHint.translated(offset));
        if (count()) {
            proxyWidget->show();
            proxyWidget->raise();
        } else
            proxyWidget->hide();
    } else {
        setGeometry(rectOverlay.translated(offset));

        for(int i=0, count=splitter->count(); i<count; ++i)
            splitter->widget(i)->show();

        if(!isVisible() && count()) {
            proxyWidget->hide();
            startShow();
        }
    }
}

void OverlayTabWidget::addWidget(QDockWidget *dock, const QString &title)
{
    if (!getMainWindow() || !getMainWindow()->getMdiArea())
        return;

    OverlayManager::instance()->registerDockWidget(dock->objectName(), this);

    setFocusView();
    getMainWindow()->removeDockWidget(dock);

    auto titleWidget = dock->titleBarWidget();
    if(titleWidget && titleWidget->objectName()==QStringLiteral("OverlayTitle")) {
        // replace the title bar with an invisible widget to hide it. The
        // OverlayTabWidget uses its own title bar for all docks.
        auto w = new QWidget();
        w->setObjectName(QStringLiteral("OverlayTitle"));
        dock->setTitleBarWidget(w);
        w->hide();
        titleWidget->deleteLater();
    }

    dock->show();
    splitter->addWidget(dock);
    auto dummyWidget = new QWidget(this);
    addTab(dummyWidget, title);
    connect(dock, SIGNAL(destroyed(QObject*)), dummyWidget, SLOT(deleteLater()));

    dock->setFeatures(dock->features() & ~QDockWidget::DockWidgetFloatable);
    if(count() == 1) {
        QRect rect = dock->geometry();
        QSize sizeMain = getMainWindow()->getMdiArea()->size();
        switch(dockArea) {
        case Qt::LeftDockWidgetArea:
        case Qt::RightDockWidgetArea:
            if (rect.width() > sizeMain.width()/3)
                rect.setWidth(sizeMain.width()/3);
            break;
        case Qt::TopDockWidgetArea:
        case Qt::BottomDockWidgetArea:
            if (rect.height() > sizeMain.height()/3)
                rect.setHeight(sizeMain.height()/3);
            break;
        default:
            break;
        }
        setRect(rect);
    }

    saveTabs();
}

int OverlayTabWidget::dockWidgetIndex(QDockWidget *dock) const
{
    return splitter->indexOf(dock);
}

void OverlayTabWidget::removeWidget(QDockWidget *dock, QDockWidget *lastDock)
{
    int index = dockWidgetIndex(dock);
    if(index < 0)
        return;

    OverlayManager::instance()->unregisterDockWidget(dock->objectName(), this);

    setFocusView();
    dock->show();
    if(lastDock)
        getMainWindow()->tabifyDockWidget(lastDock, dock);
    else
        getMainWindow()->addDockWidget(dockArea, dock);

    auto w = this->widget(index);
    removeTab(index);
    w->deleteLater();

    if(!count())
        hide();

    w = dock->titleBarWidget();
    if(w && w->objectName() == QStringLiteral("OverlayTitle")) {
        dock->setTitleBarWidget(nullptr);
        w->deleteLater();
    }
    OverlayManager::instance()->setupTitleBar(dock);

    dock->setFeatures(dock->features() | QDockWidget::DockWidgetFloatable);

    setOverlayMode(dock, 0);

    saveTabs();
}

void OverlayTabWidget::resizeEvent(QResizeEvent *ev)
{
    QTabWidget::resizeEvent(ev);
    if (_state <= State_Normal)
        timer.start(10);
}

void OverlayTabWidget::setupLayout()
{
    if (_state > State_Normal)
        return;

    if(count() == 1)
        tabSize = 0;
    else {
        int tsize;
        if(dockArea==Qt::LeftDockWidgetArea || dockArea==Qt::RightDockWidgetArea)
            tsize = tabBar()->width();
        else
            tsize = tabBar()->height();
        tabSize = tsize;
    }
    int titleBarSize = widgetMinSize(this, true);
    QRect rect, rectTitle;
    switch(tabPosition()) {
    case West:
        rectTitle = QRect(tabSize, 0, this->width()-tabSize, titleBarSize);
        rect = QRect(rectTitle.left(), rectTitle.bottom(),
                     rectTitle.width(), this->height()-rectTitle.height());
        break;
    case East:
        rectTitle = QRect(0, 0, this->width()-tabSize, titleBarSize);
        rect = QRect(rectTitle.left(), rectTitle.bottom(),
                     rectTitle.width(), this->height()-rectTitle.height());
        break;
    case North:
        rectTitle = QRect(0, tabSize, titleBarSize, this->height()-tabSize);
        rect = QRect(rectTitle.right(), rectTitle.top(),
                     this->width()-rectTitle.width(), rectTitle.height());
        break;
    case South:
        rectTitle = QRect(0, 0, titleBarSize, this->height()-tabSize);
        rect = QRect(rectTitle.right(), rectTitle.top(),
                     this->width()-rectTitle.width(), rectTitle.height());
        break;
    }
    if (_animation != 0.0) {
        switch(dockArea) {
        case Qt::LeftDockWidgetArea:
            rect.moveLeft(rect.left() - _animation * rect.width());
            break;
        case Qt::RightDockWidgetArea:
            rect.moveLeft(rect.left() + _animation * rect.width());
            break;
        case Qt::TopDockWidgetArea:
            rect.moveTop(rect.top() - _animation * rect.height());
            break;
        case Qt::BottomDockWidgetArea:
            rect.moveTop(rect.top() + _animation * rect.height());
            break;
        default:
            break;
        }
    }
    splitter->setGeometry(rect);
    titleBar->setGeometry(rectTitle);
}

void OverlayTabWidget::setCurrent(QDockWidget *widget)
{
    int index = dockWidgetIndex(widget);
    if(index >= 0) 
        setCurrentIndex(index);
}

void OverlayTabWidget::onSplitterResize(int index)
{
    const auto &sizes = splitter->sizes();
    if (index >= 0 && index < sizes.count()) {
        if (sizes[index] == 0) {
            if (currentIndex() == index) {
                bool done = false;
                for (int i=index+1; i<sizes.count(); ++i) {
                    if (sizes[i] > 0) {
                        setCurrentIndex(i);
                        done = true;
                        break;
                    }
                }
                if (!done) {
                    for (int i=index-1; i>=0 ;--i) {
                        if (sizes[i] > 0) {
                            setCurrentIndex(i);
                            break;
                        }
                    }
                }
            }
        } else
            setCurrentIndex(index);
    }

    saveTabs();
}

void OverlayTabWidget::onCurrentChanged(int index)
{
    setState(State_Showing);

    auto sizes = splitter->sizes();
    int i=0;
    int size = splitter->orientation()==Qt::Vertical ? 
                    height()-tabBar()->height() : width()-tabBar()->width();
    for(auto &s : sizes) {
        if(i++ == index)
            s = size;
        else
            s = 0;
    }
    splitter->setSizes(sizes);
    onSplitterResize(index);
    saveTabs();
}

void OverlayTabWidget::changeSize(int changes, bool checkModify)
{
    auto modifier = checkModify ? QApplication::queryKeyboardModifiers() : Qt::NoModifier;
    if(modifier== Qt::ShiftModifier) {
        setOffset(QSize(std::max(offset.width()+changes, 0), offset.height()));
        return;
    } else if ((modifier == (Qt::ShiftModifier | Qt::AltModifier))
            || (modifier == (Qt::ShiftModifier | Qt::ControlModifier))) {
        setOffset(QSize(offset.width(), std::max(offset.height()+changes, 0)));
        return;
    } else if (modifier == Qt::ControlModifier || modifier == Qt::AltModifier) {
        setSizeDelta(sizeDelta - changes);
        return;
    }

    QRect rect = rectOverlay;
    switch(dockArea) {
    case Qt::LeftDockWidgetArea:
        rect.setRight(rect.right() + changes);
        break;
    case Qt::RightDockWidgetArea:
        rect.setLeft(rect.left() - changes);
        break;
    case Qt::TopDockWidgetArea:
        rect.setBottom(rect.bottom() + changes);
        break;
    case Qt::BottomDockWidgetArea:
        rect.setTop(rect.top() - changes);
        break;
    default:
        break;
    }
    setRect(rect);
}

void OverlayTabWidget::onSizeGripMove(const QPoint &p)
{
    if (!getMainWindow() || !getMainWindow()->getMdiArea())
        return;

    QPoint pos = mapFromGlobal(p) + this->pos();
    QPoint offset = getMainWindow()->getMdiArea()->pos();
    QRect rect = this->rectOverlay.translated(offset);

    switch(dockArea) {
    case Qt::LeftDockWidgetArea:
        if (pos.x() - rect.left() < _MinimumOverlaySize)
            return;
        rect.setRight(pos.x());
        break;
    case Qt::RightDockWidgetArea:
        if (rect.right() - pos.x() < _MinimumOverlaySize)
            return;
        rect.setLeft(pos.x());
        break;
    case Qt::TopDockWidgetArea:
        if (pos.y() - rect.top() < _MinimumOverlaySize)
            return;
        rect.setBottom(pos.y());
        break;
    default:
        if (rect.bottom() - pos.y() < _MinimumOverlaySize)
            return;
        rect.setTop(pos.y());
        break;
    }
    this->setRect(rect.translated(-offset));
    OverlayManager::instance()->refresh();
}

// -----------------------------------------------------------

OverlayTitleBar::OverlayTitleBar(QWidget * parent)
    :QWidget(parent) 
{
    setFocusPolicy(Qt::ClickFocus);
    setMouseTracking(true);
    setCursor(Qt::OpenHandCursor);
}

void OverlayTitleBar::setTitleItem(QLayoutItem *item)
{
    titleItem = item;
}

void OverlayTitleBar::paintEvent(QPaintEvent *)
{
    if (!titleItem)
        return;

    QDockWidget *dock = qobject_cast<QDockWidget*>(parentWidget());
    int vertical = false;
    int flags = Qt::AlignCenter;
    if (!dock) {
        OverlayTabWidget *tabWidget = qobject_cast<OverlayTabWidget*>(parentWidget());
        if (tabWidget) {
            switch(tabWidget->getDockArea()) {
            case Qt::TopDockWidgetArea:
                vertical = true;
            // fallthrough
            case Qt::RightDockWidgetArea:
                flags = Qt::AlignRight;
                break;
            case Qt::BottomDockWidgetArea:
                vertical = true;
            // fallthrough
            case Qt::LeftDockWidgetArea:
                flags = Qt::AlignLeft;
                break;
            default:
                break;
            }
            dock = tabWidget->dockWidget(0);
        }
    }
    if (!dock)
        return;

    QPainter painter(this);
    if (qobject_cast<OverlayTabWidget*>(parentWidget()))
        painter.fillRect(this->rect(), painter.background());

    QRect r = titleItem->geometry();
    if (vertical) {
        r = r.transposed();
        painter.translate(r.left(), r.top() + r.width());
        painter.rotate(-90);
        painter.translate(-r.left(), -r.top());
    }

    QString title;
    if (OverlayManager::instance()->isMouseTransparent()) {
        if (timerId == 0)
            timerId = startTimer(500);
        title = blink ? tr("Mouse pass through, ESC to stop") : dock->windowTitle();
    } else {
        if (timerId != 0) {
            killTimer(timerId);
            timerId = 0;
        }
        title = dock->windowTitle();
    }
    QString text = painter.fontMetrics().elidedText(
            title, Qt::ElideRight, r.width());
    painter.drawText(r, flags, text);
}

void OverlayTitleBar::timerEvent(QTimerEvent *ev)
{
    if (timerId == ev->timerId()) {
        update();
        blink = !blink;
    }
}

static inline bool
isNear(const QPoint &a, const QPoint &b, int tol = 16)
{
    QPoint d = a - b;
    return d.x()*d.x() + d.y()*d.y() < tol;
}

void OverlayTitleBar::endDrag()
{
    if (_Dragging == this) {
        _Dragging = nullptr;
        setCursor(Qt::OpenHandCursor);
        if (_DragFrame)
            _DragFrame->hide();
        if (_DragFloating)
            _DragFrame->hide();
    }
}

void OverlayTitleBar::mouseMoveEvent(QMouseEvent *me)
{
    if (ignoreMouse) {
        if (!(me->buttons() & Qt::LeftButton))
            ignoreMouse = false;
        else {
            me->ignore();
            return;
        }
    }

    if (_Dragging != this && mouseMovePending && (me->buttons() & Qt::LeftButton)) {
        if (isNear(dragOffset, me->pos()))
            return;
        mouseMovePending = false;
        _Dragging = this;
    }

    if (_Dragging != this)
        return;

    if (!(me->buttons() & Qt::LeftButton)) {
        endDrag();
        return;
    }
    OverlayManager::instance()->dragDockWidget(me->globalPos(),
                                               parentWidget(),
                                               dragOffset,
                                               dragSize);
}

void OverlayTitleBar::mousePressEvent(QMouseEvent *me)
{
    mouseMovePending = false;
    QWidget *parent = parentWidget();
    if (_Dragging || !parent || !getMainWindow() || me->button() != Qt::LeftButton)
        return;

    dragSize = parent->size();
    OverlayTabWidget *tabWidget = qobject_cast<OverlayTabWidget*>(parent);
    if (!tabWidget) {
        if(QApplication::queryKeyboardModifiers() == Qt::ShiftModifier) {
            ignoreMouse = true;
            me->ignore();
            return;
        }
    }
    else {
        for (int s : tabWidget->getSplitter()->sizes()) {
            if (!s)
                continue;
            if (tabWidget == _TopOverlay || tabWidget == _BottomOverlay) {
                dragSize.setWidth(s + this->width());
                dragSize.setHeight(tabWidget->height());
            }
            else {
                dragSize.setHeight(s + this->height());
                dragSize.setWidth(tabWidget->width());
            }
        }
    }
    ignoreMouse = false;
    QSize mwSize = getMainWindow()->size();
    dragSize.setWidth(std::max(_MinimumOverlaySize,
                               std::min(mwSize.width()/2, dragSize.width())));
    dragSize.setHeight(std::max(_MinimumOverlaySize,
                                std::min(mwSize.height()/2, dragSize.height())));

    dragOffset = me->pos();
    setCursor(Qt::ClosedHandCursor);
    mouseMovePending = true;
}

void OverlayTitleBar::mouseReleaseEvent(QMouseEvent *me)
{
    if (ignoreMouse) {
        me->ignore();
        return;
    }

    setCursor(Qt::OpenHandCursor);
    mouseMovePending = false;
    if (_Dragging != this)
        return;

    if (me->button() != Qt::LeftButton)
        return;

    _Dragging = nullptr;
    OverlayManager::instance()->dragDockWidget(me->globalPos(), 
                                               parentWidget(),
                                               dragOffset,
                                               dragSize,
                                               true);
    if (_DragFrame)
        _DragFrame->hide();
    if (_DragFloating)
        _DragFloating->hide();
}

void OverlayTitleBar::keyPressEvent(QKeyEvent *ke)
{
    if (_Dragging == this && ke->key() == Qt::Key_Escape)
        endDrag();
}


// -----------------------------------------------------------

OverlayDragFrame::OverlayDragFrame(QWidget * parent)
    :QWidget(parent)
{
}

void OverlayDragFrame::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.drawRect(0, 0, this->width()-1, this->height()-1);
    painter.setOpacity(0.3);
    painter.setBrush(QBrush(Qt::blue));
    painter.drawRect(0, 0, this->width()-1, this->height()-1);
}

QSize OverlayDragFrame::sizeHint() const
{
    return size();
}

QSize OverlayDragFrame::minimumSizeHint() const
{
    return minimumSize();
}

// -----------------------------------------------------------

OverlaySizeGrip::OverlaySizeGrip(QWidget * parent, bool vertical)
    :QWidget(parent), vertical(vertical)
{
    if (vertical) {
        this->setFixedHeight(6);
        this->setMinimumWidth(widgetMinSize(this,true));
        this->setCursor(Qt::SizeVerCursor);
    }
    else {
        this->setFixedWidth(6);
        this->setMinimumHeight(widgetMinSize(this,true));
        this->setCursor(Qt::SizeHorCursor);
    }
    setMouseTracking(true);
}

void OverlaySizeGrip::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setPen(Qt::transparent);
    painter.setOpacity(0.5);
    painter.setBrush(QBrush(Qt::black, Qt::Dense6Pattern));
    QRect rect(this->rect());
    painter.drawRect(rect);
}

void OverlaySizeGrip::mouseMoveEvent(QMouseEvent *me)
{
    if ((me->buttons() & Qt::LeftButton))
        Q_EMIT dragMove(me->globalPos());
}

void OverlaySizeGrip::mousePressEvent(QMouseEvent *)
{
}

void OverlaySizeGrip::mouseReleaseEvent(QMouseEvent *)
{
}

// -----------------------------------------------------------

OverlaySplitter::OverlaySplitter(QWidget *parent)
    : QSplitter(parent)
{
}

QSplitterHandle * OverlaySplitter::createHandle()
{
    auto widget = new OverlaySplitterHandle(this->orientation(), this);

    QBoxLayout *layout = nullptr;
    auto tabWidget = qobject_cast<OverlayTabWidget*>(parentWidget());
    if(!tabWidget) {
        layout = new QBoxLayout(QBoxLayout::LeftToRight, widget); 
    } else {
        switch(tabWidget->getDockArea()) {
        case Qt::LeftDockWidgetArea:
            layout = new QBoxLayout(QBoxLayout::LeftToRight, widget); 
            break;
        case Qt::RightDockWidgetArea:
            layout = new QBoxLayout(QBoxLayout::RightToLeft, widget); 
            break;
        case Qt::TopDockWidgetArea:
            layout = new QBoxLayout(QBoxLayout::TopToBottom, widget); 
            break;
        case Qt::BottomDockWidgetArea:
            layout = new QBoxLayout(QBoxLayout::BottomToTop, widget); 
            break;
        default:
            break;
        }
    }
    bool vertical = this->orientation() != Qt::Vertical;
    layout->addSpacing(5);
    layout->setContentsMargins(1,1,1,1);
    int buttonSize = widgetMinSize(this);
    auto spacer = new QSpacerItem(buttonSize,buttonSize,
                vertical?QSizePolicy::Minimum:QSizePolicy::Expanding,
                vertical?QSizePolicy::Expanding:QSizePolicy::Minimum);
    layout->addSpacerItem(spacer);
    widget->setTitleItem(spacer);

    layout->addWidget(createTitleButton(&widget->actFloat, widgetMinSize(widget)));

    if (tabWidget) {
        auto grip = new OverlaySizeGrip(tabWidget, vertical);
        QObject::connect(grip, SIGNAL(dragMove(QPoint)),
                        tabWidget, SLOT(onSizeGripMove(QPoint)));
        layout->addWidget(grip);
        grip->raise();
    }

    return widget;
}

// -----------------------------------------------------------

OverlaySplitterHandle::OverlaySplitterHandle(Qt::Orientation orientation, QSplitter *parent)
    : QSplitterHandle(orientation, parent)
{
    setMouseTracking(true);
    setFocusPolicy(Qt::ClickFocus);
    retranslate();
    refreshIcons();
    QObject::connect(&actFloat, SIGNAL(triggered(bool)), this, SLOT(onAction()));
    timer.setSingleShot(true);
    QObject::connect(&timer, SIGNAL(timeout()), this, SLOT(onTimer()));
}

void OverlaySplitterHandle::refreshIcons()
{
    actFloat.setIcon(BitmapFactory().pixmap("qss:overlay/float.svg"));
}

void OverlaySplitterHandle::onTimer()
{
    if (isVisible() && qApp->widgetAt(QCursor::pos()) != this)
        showTitle(false);
}

void OverlaySplitterHandle::showEvent(QShowEvent *ev)
{
    if (OverlayParams::getDockOverlaySplitterHandleTimeout() > 0
            && qApp->widgetAt(QCursor::pos()) != this)
        timer.start(OverlayParams::getDockOverlaySplitterHandleTimeout());
    QSplitterHandle::showEvent(ev);
}

void OverlaySplitterHandle::enterEvent(QEvent *ev)
{
    timer.stop();
    QSplitterHandle::enterEvent(ev);
}

void OverlaySplitterHandle::leaveEvent(QEvent *ev)
{
    if (OverlayParams::getDockOverlaySplitterHandleTimeout() > 0)
        timer.start(OverlayParams::getDockOverlaySplitterHandleTimeout());
    QSplitterHandle::leaveEvent(ev);
}

QSize OverlaySplitterHandle::sizeHint() const
{ 
    QSize size = QSplitterHandle::sizeHint();
    int minSize = widgetMinSize(this,true);
    if (this->orientation() == Qt::Vertical)
        size.setHeight(std::max(minSize, size.height()));
    else
        size.setWidth(std::max(minSize, size.width()));
    return size;
}

void OverlaySplitterHandle::onAction()
{
    auto action = qobject_cast<QAction*>(sender());
    if(action == &actFloat) {
        QDockWidget *dock = dockWidget();
        if (dock)
            OverlayManager::instance()->floatDockWidget(dock);
    }
}

QDockWidget *OverlaySplitterHandle::dockWidget()
{
    QSplitter *parent = splitter();
    if (!parent)
        return nullptr;

    if (parent->handle(this->idx) != this) {
        this->idx = -1;
        for (int i=0, c=parent->count(); i<c; ++i) {
            if (parent->handle(i) == this) {
                this->idx = i;
                break;
            }
        }
    }
    return qobject_cast<QDockWidget*>(parent->widget(this->idx));
}

void OverlaySplitterHandle::retranslate()
{
    actFloat.setToolTip(QObject::tr("Toggle floating window"));
}

void OverlaySplitterHandle::changeEvent(QEvent *e)
{
    QSplitterHandle::changeEvent(e);
    if (e->type() == QEvent::LanguageChange)
        retranslate();
}

void OverlaySplitterHandle::setTitleItem(QLayoutItem *item)
{
    titleItem = item;
}

void OverlaySplitterHandle::showTitle(bool enable)
{
    if (_showTitle == enable)
        return;
    if (!enable)
        unsetCursor();
    else {
        setCursor(this->orientation() == Qt::Horizontal
                ?  Qt::SizeHorCursor : Qt::SizeVerCursor);
        if (OverlayParams::getDockOverlaySplitterHandleTimeout() > 0
                && qApp->widgetAt(QCursor::pos()) != this)
            timer.start(OverlayParams::getDockOverlaySplitterHandleTimeout());
    }
    _showTitle = enable;
    for (auto child : findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly))
        child->setVisible(enable);
    update();
}

void OverlaySplitterHandle::paintEvent(QPaintEvent *e)
{
    if (!_showTitle)
        return;

    if (!titleItem) {
        QSplitterHandle::paintEvent(e);
        return;
    }

    int flags = Qt::AlignCenter;
    auto tabWidget = qobject_cast<OverlayTabWidget*>(
            splitter() ? splitter()->parentWidget() : nullptr);

    if (tabWidget) {
        switch(tabWidget->getDockArea()) {
        case Qt::TopDockWidgetArea:
        case Qt::RightDockWidgetArea:
            flags = Qt::AlignRight;
            break;
        case Qt::BottomDockWidgetArea:
        case Qt::LeftDockWidgetArea:
            flags = Qt::AlignLeft;
            break;
        default:
            break;
        }
    }

    QDockWidget *dock = dockWidget();
    if (!dock) {
        QSplitterHandle::paintEvent(e);
        return;
    }
    
    QPainter painter(this);
    painter.fillRect(this->rect(), painter.background());

    QRect r = titleItem->geometry();
    if (this->orientation() != Qt::Vertical) {
        r = r.transposed();
        painter.translate(r.left(), r.top() + r.width());
        painter.rotate(-90);
        painter.translate(-r.left(), -r.top());
    }
    QString text = painter.fontMetrics().elidedText(
            dock->windowTitle(), Qt::ElideRight, r.width());

    painter.drawText(r, flags, text);
}

void OverlaySplitterHandle::endDrag()
{
    auto tabWidget = qobject_cast<OverlayTabWidget*>(splitter()->parentWidget());
    if (tabWidget) {
        dockWidget();
        tabWidget->onSplitterResize(this->idx);
    }
    _Dragging = nullptr;
    setCursor(this->orientation() == Qt::Horizontal
            ?  Qt::SizeHorCursor : Qt::SizeVerCursor);
    if (_DragFrame)
        _DragFrame->hide();
    if (_DragFloating)
        _DragFloating->hide();
}

void OverlaySplitterHandle::keyPressEvent(QKeyEvent *ke)
{
    if (_Dragging == this && ke->key() == Qt::Key_Escape)
        endDrag();
}

void OverlaySplitterHandle::mouseMoveEvent(QMouseEvent *me)
{
    if (_Dragging != this)
        return;

    if (!(me->buttons() & Qt::LeftButton)) {
        endDrag();
        return;
    }

    if (dragging == 1) {
        OverlayTabWidget *overlay = qobject_cast<OverlayTabWidget*>(
                splitter()->parentWidget());
        QPoint pos = me->pos();
        if (overlay) {
            switch(overlay->getDockArea()) {
            case Qt::LeftDockWidgetArea:
            case Qt::RightDockWidgetArea:
                if (pos.x() < 0 || pos.x() > overlay->width())
                    dragging = 2;
                break;
            case Qt::TopDockWidgetArea:
            case Qt::BottomDockWidgetArea:
                if (pos.y() < 0 || pos.y() > overlay->height())
                    dragging = 2;
                break;
            default:
                break;
            }
        }
        if (dragging == 1) {
            QPoint offset = parentWidget()->mapFromGlobal(me->globalPos()) - dragOffset;
            moveSplitter(this->orientation() == Qt::Horizontal ? offset.x() : offset.y());
            return;
        }
        setCursor(Qt::ClosedHandCursor);
    }

    OverlayManager::instance()->dragDockWidget(me->globalPos(),
                                               dockWidget(),
                                               dragOffset,
                                               dragSize);
}

void OverlaySplitterHandle::mousePressEvent(QMouseEvent *me)
{
    if (_Dragging || !getMainWindow() || me->button() != Qt::LeftButton)
        return;

    _Dragging = this;
    dragging = 1;
    dragOffset = me->pos();
    auto dock = dockWidget();
    if (dock) {
        dragSize = dock->size();
        dock->show();
    } else
        dragSize = QSize();

    QSize mwSize = getMainWindow()->size();
    dragSize.setWidth(std::max(_MinimumOverlaySize,
                               std::min(mwSize.width()/2, dragSize.width())));
    dragSize.setHeight(std::max(_MinimumOverlaySize,
                                std::min(mwSize.height()/2, dragSize.height())));

}

void OverlaySplitterHandle::mouseReleaseEvent(QMouseEvent *me)
{
    if (_Dragging != this || me->button() != Qt::LeftButton) 
        return;

    if (dragging == 1) {
        endDrag();
        return;
    }
    endDrag();
    OverlayManager::instance()->dragDockWidget(me->globalPos(), 
                                                dockWidget(),
                                                dragOffset,
                                                dragSize,
                                                true);
    // Warning! the handle itself maybe deleted after return from
    // dragDockWidget().
}

// -----------------------------------------------------------

OverlayGraphicsEffect::OverlayGraphicsEffect(QObject *parent) :
    QGraphicsEffect(parent),
    _enabled(false),
    _size(1,1),
    _blurRadius(2.0f),
    _color(0, 0, 0, 80)
{
}

QT_BEGIN_NAMESPACE
  extern Q_WIDGETS_EXPORT void qt_blurImage(QPainter *p, QImage &blurImage, qreal radius, bool quality, bool alphaOnly, int transposed = 0 );
QT_END_NAMESPACE

void OverlayGraphicsEffect::draw(QPainter* painter)
{
    // if nothing to show outside the item, just draw source
    if (!_enabled || _blurRadius + _size.height() <= 0 || _blurRadius + _size.width() <= 0) {
        drawSource(painter);
        return;
    }

    PixmapPadMode mode = QGraphicsEffect::PadToEffectiveBoundingRect;
    QPoint offset;
    QPixmap px = sourcePixmap(Qt::DeviceCoordinates, &offset, mode);

    // return if no source
    if (px.isNull())
        return;

#if 0
    if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG)) {
        static int count;
        getMainWindow()->showMessage(
                QStringLiteral("dock overlay redraw %1").arg(count++));
    }
#endif

    QTransform restoreTransform = painter->worldTransform();
    painter->setWorldTransform(QTransform());

    // Calculate size for the background image
    QImage tmp(px.size(), QImage::Format_ARGB32_Premultiplied);
    tmp.setDevicePixelRatio(px.devicePixelRatioF());
    tmp.fill(0);
    QPainter tmpPainter(&tmp);
    QPainterPath clip;
    tmpPainter.setCompositionMode(QPainter::CompositionMode_Source);
    if(_size.width() == 0 && _size.height() == 0)
        tmpPainter.drawPixmap(QPoint(0, 0), px);
    else {
        // exclude splitter handles
        auto splitter = qobject_cast<QSplitter*>(parent());
        if (splitter) {
            int i = -1;
            for (int size : splitter->sizes()) {
                ++i;
                if (!size)
                    continue;
                QWidget *w = splitter->widget(i);
                if (w->findChild<TaskView::TaskView*>())
                    continue;
                QRect rect = w->geometry();
                if (splitter->orientation() == Qt::Vertical)
                    clip.addRect(rect.x(), rect.y()+4,
                                rect.width(), rect.height()-4);
                else
                    clip.addRect(rect.x()+4, rect.y(),
                                rect.width()-4, rect.height());
            }
            if (clip.isEmpty()) {
                drawSource(painter);
                return;
            }
            tmpPainter.setClipPath(clip);
        }

        for (int x=-_size.width();x<=_size.width();++x) {
            for (int y=-_size.height();y<=_size.height();++y) {
                if (x || y) {
                    tmpPainter.drawPixmap(QPoint(x, y), px);
                    tmpPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);
                }
            }
        }
    }
    tmpPainter.end();

    // blur the alpha channel
    QImage blurred(tmp.size(), QImage::Format_ARGB32_Premultiplied);
    blurred.setDevicePixelRatio(px.devicePixelRatioF());
    blurred.fill(0);
    QPainter blurPainter(&blurred);
    qt_blurImage(&blurPainter, tmp, blurRadius(), false, true);
    blurPainter.end();

    tmp = blurred;

    // blacken the image...
    tmpPainter.begin(&tmp);
    tmpPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    tmpPainter.fillRect(tmp.rect(), color());
    tmpPainter.end();

    // draw the blurred shadow...
    painter->drawImage(QPointF(offset.x()+_offset.x(), offset.y()+_offset.y()), tmp);

    // draw the actual pixmap...
    painter->drawPixmap(offset, px, QRectF());

#if 0
    QWidget *focus = qApp->focusWidget();
    if (focus) {
        QWidget *widget = qobject_cast<QWidget*>(this->parent());
        if (auto *edit = qobject_cast<QPlainTextEdit*>(focus)) {
            if (!edit->isReadOnly() && edit->isEnabled()) {
                for(auto w=edit->parentWidget(); w; w=w->parentWidget()) {
                    if (w == widget) {
                        QRect r = edit->cursorRect();
                        QRect rect(edit->viewport()->mapTo(widget, r.topLeft()), 
                                edit->viewport()->mapTo(widget, r.bottomRight()));
                        // painter->fillRect(rect, edit->textColor());
                        // painter->fillRect(rect, edit->currentCharFormat().foreground());
                        painter->fillRect(rect.translated(offset), Qt::white);
                    }
                }
            }
        }
    }
#endif

    // restore world transform
    painter->setWorldTransform(restoreTransform);
}

QRectF OverlayGraphicsEffect::boundingRectFor(const QRectF& rect) const
{
    if (!_enabled)
        return rect;
    return rect.united(rect.adjusted(-_blurRadius - _size.width() + _offset.x(), 
                                     -_blurRadius - _size.height()+ _offset.y(), 
                                     _blurRadius + _size.width() + _offset.x(),
                                     _blurRadius + _size.height() + _offset.y()));
}

// -----------------------------------------------------------

struct OverlayInfo {
    const char *name;
    OverlayTabWidget *tabWidget;
    Qt::DockWidgetArea dockArea;
    std::unordered_map<QDockWidget*, OverlayInfo*> &overlayMap;
    ParameterGrp::handle hGrp;
    boost::signals2::scoped_connection conn;

    OverlayInfo(QWidget *parent,
                const char *name,
                Qt::DockWidgetArea pos,
                std::unordered_map<QDockWidget*, OverlayInfo*> &map)
        : name(name), dockArea(pos), overlayMap(map)
    {
        tabWidget = new OverlayTabWidget(parent, dockArea);
        tabWidget->setObjectName(QString::fromUtf8(name));
        tabWidget->getProxyWidget()->setObjectName(tabWidget->objectName() + QStringLiteral("Proxy"));
        tabWidget->setMovable(true);
        hGrp = App::GetApplication().GetUserParameter().GetGroup("BaseApp")
                            ->GetGroup("MainWindow")->GetGroup("DockWindows")->GetGroup(name);
        conn = App::GetApplication().GetUserParameter().signalParamChanged.connect(
            [this](ParameterGrp *Param, ParameterGrp::ParamType, const char *Name, const char *) {
                if (hGrp == Param && Name && !tabWidget->isSaving()) {
                    // This will prevent saving settings which will mess up the
                    // just restored ones
                    tabWidget->restore(nullptr); 
                    OverlayManager::instance()->reload();
                }
            });
    }

    bool addWidget(QDockWidget *dock, bool forced=true) {
        if(!dock)
            return false;
        if(tabWidget->dockWidgetIndex(dock) >= 0)
            return false;
        overlayMap[dock] = this;
        bool visible = dock->isVisible();

        auto focus = qApp->focusWidget();
        if(focus && findTabWidget(focus) != tabWidget)
            focus = nullptr;

        tabWidget->addWidget(dock, dock->windowTitle());

        if(focus) {
            tabWidget->setCurrent(dock);
            focus = qApp->focusWidget();
            if(focus)
                focus->clearFocus();
        }

        if(forced) {
            auto mw = getMainWindow();
            for(auto d : mw->findChildren<QDockWidget*>()) {
                if(mw->dockWidgetArea(d) == dockArea
                        && d->toggleViewAction()->isChecked())
                {
                    addWidget(d, false);
                }
            }
            if(visible) {
                dock->show();
                tabWidget->setCurrent(dock);
            }
        } else
            tabWidget->saveTabs();
        return true;
    }

    void removeWidget() {
        if(!tabWidget->count())
            return;

        tabWidget->hide();

        QPointer<QWidget> focus = qApp->focusWidget();

        QDockWidget *lastDock = tabWidget->currentDockWidget();
        if(lastDock)
            tabWidget->removeWidget(lastDock);
        while(tabWidget->count()) {
            QDockWidget *dock = tabWidget->dockWidget(0);
            if(!dock) {
                tabWidget->removeTab(0);
                continue;
            }
            tabWidget->removeWidget(dock, lastDock);
            lastDock = dock;
        }

        if(focus)
            focus->setFocus();

        tabWidget->saveTabs();
    }

    void save()
    {
    }

    void restore()
    {
        tabWidget->restore(hGrp);
        for(int i=0,c=tabWidget->count();i<c;++i) {
            auto dock = tabWidget->dockWidget(i);
            if(dock)
                overlayMap[dock] = this;
        }
    }
};

enum OverlayToggleMode {
    OverlayUnset,
    OverlaySet,
    OverlayToggle,
    OverlayToggleTransparent,
    OverlayCheck,
};

class OverlayManager::Private {
public:

    QPointer<QWidget> lastIntercept;
    QTimer _timer;
    QTimer _reloadTimer;

    bool mouseTransparent = false;
    bool intercepting = false;

    std::unordered_map<QDockWidget*, OverlayInfo*> _overlayMap;
    OverlayInfo _left;
    OverlayInfo _right;
    OverlayInfo _top;
    OverlayInfo _bottom;
    std::array<OverlayInfo*,4> _overlayInfos;
    QCursor _cursor;

    QPoint _lastPos;

    QAction _actClose;
    QAction _actFloat;
    QAction _actOverlay;
    std::array<QAction*, 3> _actions;

    QPointer<QWidget> _trackingWidget;
    OverlayTabWidget *_trackingOverlay = nullptr;

    bool updateStyle = false;
    QTime wheelDelay;
    QPoint wheelPos;

    std::map<QString, OverlayTabWidget*> _dockWidgetNameMap;

    bool raising = false;

    OverlayManager::ReloadMode curReloadMode = OverlayManager::ReloadPending;

    Private(OverlayManager *host, QWidget *parent)
        :_left(parent,"OverlayLeft", Qt::LeftDockWidgetArea,_overlayMap)
        ,_right(parent,"OverlayRight", Qt::RightDockWidgetArea,_overlayMap)
        ,_top(parent,"OverlayTop", Qt::TopDockWidgetArea,_overlayMap)
        ,_bottom(parent,"OverlayBottom",Qt::BottomDockWidgetArea,_overlayMap)
        ,_overlayInfos({&_left,&_right,&_top,&_bottom})
        ,_actions({&_actOverlay,&_actFloat,&_actClose})
    {
        _Overlays = {_LeftOverlay, _RightOverlay, _TopOverlay, _BottomOverlay};

        connect(&_timer, SIGNAL(timeout()), host, SLOT(onTimer()));
        _timer.setSingleShot(true);

        _reloadTimer.setSingleShot(true);
        connect(&_reloadTimer, &QTimer::timeout, [this]() {
            for (auto &o : _overlayInfos) {
                o->tabWidget->restore(nullptr); // prevent saving setting first
                o->removeWidget();
            }
            for (auto &o : _overlayInfos)
                o->restore();
            refresh();
        });

        connect(qApp, SIGNAL(focusChanged(QWidget*,QWidget*)),
                host, SLOT(onFocusChanged(QWidget*,QWidget*)));

        Application::Instance->signalActivateView.connect([this](const MDIView *) {
            refresh();
        });
        Application::Instance->signalInEdit.connect([this](const ViewProviderDocumentObject &) {
            refresh();
        });
        Application::Instance->signalResetEdit.connect([this](const ViewProviderDocumentObject &) {
            refresh();
        });

        _actOverlay.setData(QStringLiteral("OBTN Overlay"));
        _actFloat.setData(QStringLiteral("OBTN Float"));
        _actClose.setData(QStringLiteral("OBTN Close"));

        retranslate();
        refreshIcons();

        for(auto action : _actions) {
            QObject::connect(action, SIGNAL(triggered(bool)), host, SLOT(onAction()));
        }
        for(auto o : _overlayInfos) {
            for(auto action : o->tabWidget->actions()) {
                QObject::connect(action, SIGNAL(triggered(bool)), host, SLOT(onAction()));
            }
            o->tabWidget->setTitleBar(createTitleBar(o->tabWidget));
        }

        QIcon px = BitmapFactory().pixmap("cursor-through");
        _cursor = QCursor(px.pixmap(32,32), 10, 9);
    }

    void refreshIcons()
    {
        _actFloat.setIcon(BitmapFactory().pixmap("qss:overlay/float.svg"));
        _actOverlay.setIcon(BitmapFactory().pixmap("qss:overlay/overlay.svg"));
        _actClose.setIcon(BitmapFactory().pixmap("qss:overlay/close.svg"));
        for (OverlayTabWidget *tabWidget : _Overlays) {
            tabWidget->refreshIcons();
            for (auto handle : tabWidget->findChildren<OverlaySplitterHandle*>())
                handle->refreshIcons();
        }
    }

    void interceptEvent(QWidget *, QEvent *);

    void setMouseTransparent(bool enabled)
    {
        if (mouseTransparent == enabled)
            return;
        mouseTransparent = enabled;
        for (OverlayTabWidget *tabWidget : _Overlays) {
            tabWidget->setAttribute(
                    Qt::WA_TransparentForMouseEvents, enabled);
            tabWidget->touch();
        }
        refresh();
        if(!enabled)
            qApp->restoreOverrideCursor();
        else
            qApp->setOverrideCursor(_cursor);
    }

    bool toggleOverlay(QDockWidget *dock, int toggle, int dockPos=Qt::NoDockWidgetArea)
    {
        if(!dock)
            return false;

        auto it = _overlayMap.find(dock);
        if(it != _overlayMap.end()) {
            auto o = it->second;
            switch(toggle) {
            case OverlayToggleTransparent:
                o->tabWidget->setTransparent(!o->tabWidget->isTransparent());
                break;
            case OverlayUnset:
            case OverlayToggle:
                _overlayMap.erase(it);
                o->tabWidget->removeWidget(dock);
                return false;
            default:
                break;
            }
            return true;
        }

        if(toggle == OverlayUnset)
            return false;

        if(dockPos == Qt::NoDockWidgetArea)
            dockPos = getMainWindow()->dockWidgetArea(dock);
        OverlayInfo *o;
        switch(dockPos) {
        case Qt::LeftDockWidgetArea:
            o = &_left;
            break;
        case Qt::RightDockWidgetArea:
            o = &_right;
            break;
        case Qt::TopDockWidgetArea:
            o = &_top;
            break;
        case Qt::BottomDockWidgetArea:
            o = &_bottom;
            break;
        default:
            return false;
        }
        if(toggle == OverlayCheck && !o->tabWidget->count())
            return false;
        if(o->addWidget(dock)) {
            if(toggle == OverlayToggleTransparent)
                o->tabWidget->setTransparent(true);
        }
        refresh();
        return true;
    }

    void refresh(QWidget *widget=nullptr, bool refreshStyle=false)
    {
        if(refreshStyle) {
            OverlayStyleSheet::instance()->update();
            updateStyle = true;
        }

        if(widget) {
            auto tabWidget = findTabWidget(widget);
            if(tabWidget && tabWidget->count()) {
                for(auto o : _overlayInfos) {
                    if(tabWidget == o->tabWidget) {
                        tabWidget->touch();
                        break;
                    }
                }
            }
        }
        _timer.start(OverlayParams::getDockOverlayDelay());
    }

    void save()
    {
        _left.save();
        _right.save();
        _top.save();
        _bottom.save();
    }

    void restore()
    {
        _left.restore();
        _right.restore();
        _top.restore();
        _bottom.restore();
        refresh();
    }

    void onTimer()
    {
        auto mdi = getMainWindow() ? getMainWindow()->getMdiArea() : nullptr;
        if(!mdi)
            return;

        auto focus = findTabWidget(qApp->focusWidget());
        if (focus && !focus->getSplitter()->isVisible())
            focus = nullptr;
        auto active = findTabWidget(qApp->widgetAt(QCursor::pos()));
        if (active && !active->getSplitter()->isVisible())
            active = nullptr;
        OverlayTabWidget *reveal = nullptr;

        bool updateFocus = false;
        bool updateActive = false;

        for(auto o : _overlayInfos) {
            if(o->tabWidget->isTouched() || updateStyle) {
                if(o->tabWidget == focus)
                    updateFocus = true;
                else if(o->tabWidget == active)
                    updateActive = true;
                else 
                    o->tabWidget->setOverlayMode(true);
            }
            if(!o->tabWidget->getRevealTime().isNull()) {
                if(o->tabWidget->getRevealTime()<= QTime::currentTime())
                    o->tabWidget->setRevealTime(QTime());
                else
                    reveal = o->tabWidget;
            }
        }
        updateStyle = false;

        if (focus) {
            if (focus->isOverlayed(1) || updateFocus) {
                focus->setOverlayMode(false);
                focus->raise();
                if(reveal == focus)
                    reveal = nullptr;
            } else 
                focus->updateSplitterHandles();
        }

        if(active) {
            if(active != focus && (active->isOverlayed(1) || updateActive)) 
                active->setOverlayMode(false);
            active->raise();
            if(reveal == active)
                reveal = nullptr;
        }

        if(reveal && !reveal->splitter->isVisible()) {
            reveal->setOverlayMode(false);
            reveal->raise();
        }

        for(auto o : _overlayInfos) {
            if(o->tabWidget != focus 
                    && o->tabWidget != active
                    && o->tabWidget != reveal
                    && o->tabWidget->count()
                    && !o->tabWidget->isOverlayed(-1))
            {
                o->tabWidget->setOverlayMode(true);
            }
        }

        int w = mdi->geometry().width();
        int h = mdi->geometry().height();
        auto tabbar = mdi->findChild<QTabBar*>();
        if(tabbar)
            h -= tabbar->height();

        int naviCubeSize = NaviCube::getNaviCubeSize();
        int naviCorner = OverlayParams::getDockOverlayCheckNaviCube() ?
            OverlayParams::getCornerNaviCube() : -1;

        QRect rect;
        QRect rectBottom(0,0,0,0);

        rect = _bottom.tabWidget->getRect();

        QSize ofs = _bottom.tabWidget->getOffset();
        int delta = _bottom.tabWidget->getSizeDelta();
        h -= ofs.height();

        auto getCubeSize = [naviCubeSize](OverlayInfo &info) -> int {
            float scale = info.tabWidget->_imageScale;
            if (scale == 0.0) {
                scale = info.tabWidget->titleBar->grab().devicePixelRatio();
                if (scale == 0.0)
                    scale = 1.0;
            }
            return naviCubeSize/scale + 10;
        };

        int cubeSize = getCubeSize(_bottom);
        if(naviCorner == 2)
            ofs.setWidth(ofs.width()+cubeSize);
        int bw = w-10-ofs.width()-delta;
        if(naviCorner == 3)
            bw -= cubeSize;
        if(bw < 10)
            bw = 10;

        // Bottom width is maintain the same to reduce QTextEdit re-layout
        // which may be expensive if there are lots of text, e.g. for
        // ReportView or PythonConsole.
        _bottom.tabWidget->setRect(QRect(ofs.width(),h-rect.height(),bw,rect.height()));

        if (_bottom.tabWidget->count()
                && _bottom.tabWidget->isVisible() 
                && _bottom.tabWidget->getState() <= OverlayTabWidget::State_Normal)
            rectBottom = _bottom.tabWidget->getRect();

        QRect rectLeft(0,0,0,0);
        rect = _left.tabWidget->getRect();

        ofs = _left.tabWidget->getOffset();
        cubeSize = getCubeSize(_left);
        if(naviCorner == 0)
            ofs.setWidth(ofs.width()+cubeSize);
        delta = _left.tabWidget->getSizeDelta()+rectBottom.height();
        if(naviCorner == 2 && cubeSize > rectBottom.height())
            delta += cubeSize - rectBottom.height();
        int lh = std::max(h-ofs.width()-delta, 10);

        _left.tabWidget->setRect(QRect(ofs.height(),ofs.width(),rect.width(),lh));

        if (_left.tabWidget->count()
                && _left.tabWidget->isVisible() 
                && _left.tabWidget->getState() <= OverlayTabWidget::State_Normal)
            rectLeft = _left.tabWidget->getRect();

        QRect rectRight(0,0,0,0);
        rect = _right.tabWidget->getRect();

        ofs = _right.tabWidget->getOffset();
        cubeSize = getCubeSize(_right);
        if(naviCorner == 1)
            ofs.setWidth(ofs.width()+cubeSize);
        delta = _right.tabWidget->getSizeDelta()+rectBottom.height();
        if(naviCorner == 3 && cubeSize > rectBottom.height())
            delta += cubeSize - rectBottom.height();
        int rh = std::max(h-ofs.width()-delta, 10);
        w -= ofs.height();

        _right.tabWidget->setRect(QRect(w-rect.width(),ofs.width(),rect.width(),rh));

        if (_right.tabWidget->count()
                && _right.tabWidget->isVisible() 
                && _right.tabWidget->getState() <= OverlayTabWidget::State_Normal)
            rectRight = _right.tabWidget->getRect();

        rect = _top.tabWidget->getRect();

        ofs = _top.tabWidget->getOffset();
        cubeSize = getCubeSize(_top);
        delta = _top.tabWidget->getSizeDelta();
        if(naviCorner == 0)
            rectLeft.setWidth(std::max(rectLeft.width(), cubeSize));
        else if(naviCorner == 1)
            rectRight.setWidth(std::max(rectRight.width(), cubeSize));
        int tw = w-rectLeft.width()-rectRight.width()-ofs.width()-delta;

        _top.tabWidget->setRect(QRect(rectLeft.width()-ofs.width(),ofs.height(),tw,rect.height()));
    }

    void setOverlayMode(OverlayMode mode)
    {
        switch(mode) {
        case OverlayManager::DisableAll:
        case OverlayManager::EnableAll: {
            auto docks = getMainWindow()->findChildren<QDockWidget*>();
            // put visible dock widget first
            std::sort(docks.begin(),docks.end(),
                [](const QDockWidget *a, const QDockWidget *b) {
                    return !a->visibleRegion().isEmpty() && b->visibleRegion().isEmpty();
                });
            for(auto dock : docks) {
                if(mode == OverlayManager::DisableAll)
                    toggleOverlay(dock, OverlayUnset);
                else
                    toggleOverlay(dock, OverlaySet);
            }
            return;
        }
        case OverlayManager::ToggleAll:
            for(auto o : _overlayInfos) {
                if(o->tabWidget->count()) {
                    setOverlayMode(OverlayManager::DisableAll);
                    return;
                }
            }
            setOverlayMode(OverlayManager::EnableAll);
            return;
        case OverlayManager::TransparentAll: {
            bool found = false;
            for(auto o : _overlayInfos) {
                if(o->tabWidget->count())
                    found = true;
            }
            if(!found)
                setOverlayMode(OverlayManager::EnableAll);
        }
        // fall through
        case OverlayManager::TransparentNone:
            for(auto o : _overlayInfos)
                o->tabWidget->setTransparent(mode == OverlayManager::TransparentAll);
            refresh();
            return;
        case OverlayManager::ToggleTransparentAll:
            for(auto o : _overlayInfos) {
                if(o->tabWidget->count() && o->tabWidget->isTransparent()) {
                    setOverlayMode(OverlayManager::TransparentNone);
                    return;
                }
            }
            setOverlayMode(OverlayManager::TransparentAll);
            return;
        case OverlayManager::ToggleLeft:
            if (_LeftOverlay->isVisible())
                _LeftOverlay->setState(OverlayTabWidget::State_Hidden);
            else
                _LeftOverlay->setState(OverlayTabWidget::State_Showing);
            break;
        case OverlayManager::ToggleRight:
            if (_RightOverlay->isVisible())
                _RightOverlay->setState(OverlayTabWidget::State_Hidden);
            else
                _RightOverlay->setState(OverlayTabWidget::State_Showing);
            break;
        case OverlayManager::ToggleTop:
            if (_TopOverlay->isVisible())
                _TopOverlay->setState(OverlayTabWidget::State_Hidden);
            else
                _TopOverlay->setState(OverlayTabWidget::State_Showing);
            break;
        case OverlayManager::ToggleBottom:
            if (_BottomOverlay->isVisible())
                _BottomOverlay->setState(OverlayTabWidget::State_Hidden);
            else
                _BottomOverlay->setState(OverlayTabWidget::State_Showing);
            break;
        default:
            break;
        }

        OverlayToggleMode m;
        QDockWidget *dock = nullptr;
        for(auto w=qApp->widgetAt(QCursor::pos()); w; w=w->parentWidget()) {
            dock = qobject_cast<QDockWidget*>(w);
            if(dock)
                break;
            auto tabWidget = qobject_cast<OverlayTabWidget*>(w);
            if(tabWidget) {
                dock = tabWidget->currentDockWidget();
                if(dock)
                    break;
            }
        }
        if(!dock) {
            for(auto w=qApp->focusWidget(); w; w=w->parentWidget()) {
                dock = qobject_cast<QDockWidget*>(w);
                if(dock)
                    break;
            }
        }

        switch(mode) {
        case OverlayManager::ToggleActive:
            m = OverlayToggle;
            break;
        case OverlayManager::ToggleTransparent:
            m = OverlayToggleTransparent;
            break;
        case OverlayManager::EnableActive:
            m = OverlaySet;
            break;
        case OverlayManager::DisableActive:
            m = OverlayUnset;
            break;
        default:
            return;
        }
        toggleOverlay(dock, m);
    }

    void onToggleDockWidget(QDockWidget *dock, int checked)
    {
        if(!dock)
            return;

        auto it = _overlayMap.find(dock);
        if(it == _overlayMap.end())
            return;

        OverlayTabWidget *tabWidget = it->second->tabWidget;
        int index = tabWidget->dockWidgetIndex(dock);
        if(index < 0)
            return;
        auto sizes = tabWidget->getSplitter()->sizes();
        while(index >= sizes.size())
            sizes.append(0);

        if (checked < -1)
            checked = 0;
        else if (checked == 3) {
            checked = 1;
            sizes[index] = 0; // force expand the tab in full
        } else if (checked <= 1) {
            if (sizes[index] != 0 && tabWidget->isHidden())
                checked = 1;
            else {
                // child widget inside splitter by right shouldn't been hidden, so
                // we ignore the given toggle bit, but rely on its splitter size to
                // decide.
                checked = sizes[index] == 0 ? 1 : 0;
            }
        }
        if(sizes[index]==0) {
            if (!checked)
                return;
            tabWidget->setCurrent(dock);
            tabWidget->onCurrentChanged(tabWidget->dockWidgetIndex(dock));
        } else if (!checked) {
            if (sizes[index] > 0 && sizes.size() > 1) {
                int newtotal = 0;
                int total = 0;
                auto newsizes = sizes;
                newsizes[index] = 0;
                for (int i=0; i<sizes.size(); ++i) {
                    total += sizes[i];
                    if (i != index) {
                        auto d = tabWidget->dockWidget(i);
                        auto it = tabWidget->_sizemap.find(d);
                        if (it == tabWidget->_sizemap.end())
                            newsizes[i] = 0;
                        else {
                            if (newtotal == 0)
                                tabWidget->setCurrent(d);
                            newsizes[i] = it->second;
                            newtotal += it->second;
                        }
                    }
                }
                if (!newtotal) {
                    int expand = 0;
                    for (int i=0; i<sizes.size(); ++i) {
                        if (i != index && sizes[i] > 0) {
                            ++expand;
                            break;
                        }
                    }
                    if (expand) {
                        int expansion = sizes[index];
                        int step = expansion / expand;
                        for (int i=0; i<sizes.size(); ++i) {
                            if (i == index)
                                newsizes[i] = 0;
                            else if (--expand) {
                                expansion -= step;
                                newsizes[i] += step;
                            } else {
                                newsizes[i] += expansion;
                                break;
                            }
                        }
                    }
                    newsizes[index] = 0;
                }
                tabWidget->splitter->setSizes(newsizes);
                tabWidget->saveTabs();
            }
        }
        for (int i=0; i<sizes.size(); ++i) {
            if (sizes[i])
                tabWidget->_sizemap[tabWidget->dockWidget(i)] = sizes[i];
        }
        if (checked)
            tabWidget->setRevealTime(QTime::currentTime().addMSecs(
                    OverlayParams::getDockOverlayRevealDelay()));
        refresh();
    }

    void changeOverlaySize(int changes)
    {
        auto tabWidget = findTabWidget(qApp->widgetAt(QCursor::pos()));
        if(tabWidget) {
            tabWidget->changeSize(changes, false);
            refresh();
        }
    }

    void onFocusChanged(QWidget *, QWidget *) {
        refresh();
    }

    void setupTitleBar(QDockWidget *dock)
    {
        if(!dock->titleBarWidget())
            dock->setTitleBarWidget(createTitleBar(dock));
    }

    QWidget *createTitleBar(QWidget *parent)
    {
        OverlayTitleBar *widget = new OverlayTitleBar(parent);
        widget->setObjectName(QStringLiteral("OverlayTitle"));
        bool vertical = false;
        QBoxLayout *layout = nullptr;
        auto tabWidget = qobject_cast<OverlayTabWidget*>(parent);
        if(!tabWidget) {
            layout = new QBoxLayout(QBoxLayout::LeftToRight, widget); 
        } else {
            switch(tabWidget->getDockArea()) {
            case Qt::LeftDockWidgetArea:
                layout = new QBoxLayout(QBoxLayout::LeftToRight, widget); 
                break;
            case Qt::RightDockWidgetArea:
                layout = new QBoxLayout(QBoxLayout::RightToLeft, widget); 
                break;
            case Qt::TopDockWidgetArea:
                layout = new QBoxLayout(QBoxLayout::TopToBottom, widget); 
                vertical = true;
                break;
            case Qt::BottomDockWidgetArea:
                layout = new QBoxLayout(QBoxLayout::BottomToTop, widget); 
                vertical = true;
                break;
            default:
                break;
            }
        }
        layout->addSpacing(5);
        layout->setContentsMargins(1,1,1,1);
        int buttonSize = widgetMinSize(widget);
        auto spacer = new QSpacerItem(buttonSize,buttonSize,
                    vertical?QSizePolicy::Minimum:QSizePolicy::Expanding,
                    vertical?QSizePolicy::Expanding:QSizePolicy::Minimum);
        layout->addSpacerItem(spacer);
        widget->setTitleItem(spacer);

        if(tabWidget) {
            for(auto action : tabWidget->actions())
                layout->addWidget(createTitleButton(action, buttonSize));
        } else {
            for(auto action : _actions)
                layout->addWidget(createTitleButton(action, buttonSize));
        }

        if (tabWidget) {
            auto grip = new OverlaySizeGrip(tabWidget, vertical);
            QObject::connect(grip, SIGNAL(dragMove(QPoint)),
                    tabWidget, SLOT(onSizeGripMove(QPoint)));
            layout->addWidget(grip);
            grip->raise();
        }
        return widget;
    }

    void onAction(QAction *action) {
        if(action == &_actOverlay) {
            OverlayManager::instance()->setOverlayMode(OverlayManager::ToggleActive);
        } else if(action == &_actFloat || action == &_actClose) {
            for(auto w=qApp->widgetAt(QCursor::pos());w;w=w->parentWidget()) {
                auto dock = qobject_cast<QDockWidget*>(w);
                if(!dock)
                    continue;
                setFocusView();
                if(action == &_actClose) {
                    dock->toggleViewAction()->activate(QAction::Trigger);
                } else {
                    auto it = _overlayMap.find(dock);
                    if(it != _overlayMap.end()) {
                        it->second->tabWidget->removeWidget(dock);
                        getMainWindow()->addDockWidget(it->second->dockArea, dock);
                        _overlayMap.erase(it);
                        dock->show();
                        dock->setFloating(true);
                        refresh();
                    } else 
                        dock->setFloating(!dock->isFloating());
                }
                return;
            }
        } else {
            auto tabWidget = qobject_cast<OverlayTabWidget*>(action->parent());
            if(tabWidget)
                tabWidget->onAction(action);
        }
    }

    void retranslate()
    {
        _actOverlay.setToolTip(QObject::tr("Toggle overlay"));
        _actFloat.setToolTip(QObject::tr("Toggle floating window"));
        _actClose.setToolTip(QObject::tr("Close dock window"));
    }

    void floatDockWidget(QDockWidget *dock)
    {
        setFocusView();
        auto it = _overlayMap.find(dock);
        if (it != _overlayMap.end()) {
            it->second->tabWidget->removeWidget(dock);
            _overlayMap.erase(it);
        }
        dock->setFloating(true);
        dock->show();
    }

    // Warning, the caller may be deleted during the call. So do not pass
    // parameter using reference, pass by value instead.
    void dragDockWidget(QPoint pos,
                        QWidget *srcWidget,
                        QPoint dragOffset,
                        QSize dragSize,
                        bool drop)
    {
        if (!getMainWindow())
            return;
        auto mdi = getMainWindow()->getMdiArea();
        if (!mdi)
            return;

        auto dock = qobject_cast<QDockWidget*>(srcWidget);
        if (dock && dock->isFloating())
            dock->move(pos - dragOffset);

        OverlayTabWidget *src = nullptr;
        int srcIndex = -1;
        if (dock) {
            auto it = _overlayMap.find(dock);
            if (it != _overlayMap.end()) {
                src = it->second->tabWidget;
                srcIndex = src->dockWidgetIndex(dock);
            }
        }
        else {
            src = qobject_cast<OverlayTabWidget*>(srcWidget);
            if (!src)
                return;
            for(int size : src->getSplitter()->sizes()) {
                ++srcIndex;
                if (size) {
                    dock = src->dockWidget(srcIndex);
                    break;
                }
            }
            if (!dock)
                return;
        }

        OverlayTabWidget *tabWidget = nullptr;
        int resizeOffset = 0;
        int index = -1;
        QRect rect;
        QRect rectMain(getMainWindow()->mapToGlobal(QPoint()),
                       getMainWindow()->size());
        QRect rectMdi(mdi->mapToGlobal(QPoint()), mdi->size());

        for (OverlayTabWidget *overlay : _Overlays) {
            rect = QRect(mdi->mapToGlobal(overlay->rectOverlay.topLeft()),
                                          overlay->rectOverlay.size());

            QSize size(rect.width()*3/4, rect.height()*3/4);
            QSize sideSize(rect.width()/4, rect.height()/4);

            int dockArea = overlay->getDockArea();

            if (dockArea == Qt::BottomDockWidgetArea) {
                if (pos.y() < rect.bottom() && rect.bottom() - pos.y() < sideSize.height()) {
                    rect.setTop(rect.bottom() - _MinimumOverlaySize);
                    rect.setBottom(rectMain.bottom());
                    rect.setLeft(rectMdi.left());
                    rect.setRight(rectMdi.right());
                    tabWidget = overlay;
                    index = -2;
                    break;
                }
            }
            if (dockArea == Qt::LeftDockWidgetArea) {
                if (pos.x() >= rect.left() && pos.x() - rect.left() < sideSize.width()) {
                    rect.setRight(rect.left() + _MinimumOverlaySize);
                    rect.setLeft(rectMain.left());
                    rect.setTop(rectMdi.top());
                    rect.setBottom(rectMdi.bottom());
                    tabWidget = overlay;
                    index = -2;
                    break;
                }
            }
            else if (dockArea == Qt::RightDockWidgetArea) {
                if (pos.x() < rect.right() && rect.right() - pos.x() < sideSize.width()) {
                    rect.setLeft(rect.right() - _MinimumOverlaySize);
                    rect.setRight(rectMain.right());
                    rect.setTop(rectMdi.top());
                    rect.setBottom(rectMdi.bottom());
                    tabWidget = overlay;
                    index = -2;
                    break;
                }
            }
            else if (dockArea == Qt::TopDockWidgetArea) {
                if (pos.y() >= rect.top() && pos.y() - rect.top() < sideSize.height()) {
                    rect.setBottom(rect.top() + _MinimumOverlaySize);
                    rect.setTop(rectMain.top());
                    rect.setLeft(rectMdi.left());
                    rect.setRight(rectMdi.right());
                    tabWidget = overlay;
                    index = -2;
                    break;
                }
            }

            switch(dockArea) {
            case Qt::LeftDockWidgetArea:
                rect.setWidth(size.width());
                break;
            case Qt::RightDockWidgetArea:
                rect.setLeft(rect.right() - size.width());
                break;
            case Qt::TopDockWidgetArea:
                rect.setHeight(size.height());
                break;
            default:
                rect.setTop(rect.bottom() - size.height());
                break;
            }

            if (!rect.contains(pos))
                continue;

            tabWidget = overlay;
            index = -1;
            int i = -1;

            for (int size : overlay->getSplitter()->sizes()) {
                ++i;
                auto handle = overlay->getSplitter()->handle(i);
                QWidget *w  = overlay->dockWidget(i);
                if (!handle || !w)
                    continue;
                if (handle->rect().contains(handle->mapFromGlobal(pos))) {
                    QPoint pt = handle->mapToGlobal(QPoint());
                    QSize s = handle->size();
                    if (!size)
                        size = _MinimumOverlaySize;
                    if (tabWidget != src)
                        size /= 2;
                    if (overlay->getSplitter()->orientation() == Qt::Vertical)
                        s.setHeight(s.height() + size);
                    else
                        s.setWidth(s.width() + size);
                    rect = QRect(pt, s);
                    index = i;
                    break;
                }
                if (!size)
                    continue;
                if (w->rect().contains(w->mapFromGlobal(pos))) {
                    QPoint pt = overlay->getSplitter()->mapToGlobal(w->pos());
                    rect = QRect(pt, w->size());
                    if (tabWidget != src) {
                        if (overlay->getSplitter()->orientation() == Qt::Vertical) {
                            if (pos.y() > pt.y() + size/2) {
                                rect.setTop(rect.top() + size/2);
                                resizeOffset = -1;
                                ++i;
                            }
                            else 
                                rect.setHeight(size/2);
                        }
                        else if (pos.x() > pt.x() + size/2) {
                            rect.setLeft(rect.left() + size/2);
                            resizeOffset = -1;
                            ++i;
                        }
                        else
                            rect.setWidth(size/2);
                    }
                    index = i;
                    break;
                }
            }
            break;
        };

        OverlayTabWidget *dst = nullptr;
        int dstIndex = -1;
        QDockWidget *dstDock = nullptr;
        Qt::DockWidgetArea dstDockArea;

        if (!tabWidget) {
            rect = QRect(pos - dragOffset, dragSize);
            if (rect.width() < 50)
                rect.setWidth(50);
            if (rect.height() < 50)
                rect.setHeight(50);

            for(auto dockWidget : getMainWindow()->findChildren<QDockWidget*>()) {
                if (dockWidget == dock
                        || !dockWidget->isVisible()
                        || dockWidget->isFloating()
                        || _overlayMap.count(dockWidget))
                    continue;
                if (dockWidget->rect().contains(dockWidget->mapFromGlobal(pos))) {
                    dstDock = dockWidget;
                    dstDockArea = getMainWindow()->dockWidgetArea(dstDock);
                    rect = QRect(dockWidget->mapToGlobal(QPoint()),
                                dockWidget->size());
                    break;
                }
            }
        }
        else {
            dst = tabWidget;
            dstIndex = index;
            if (dstIndex == -1)
                rect = QRect(mdi->mapToGlobal(tabWidget->rectOverlay.topLeft()),
                             tabWidget->rectOverlay.size());
        }

        bool outside = false;
        if (!rectMain.contains(pos)) {
            outside = true;
            if (drop) {
                if (!dock->isFloating()) {
                    if (src) {
                        _overlayMap.erase(dock);
                        src->removeWidget(dock);
                    }
                    setFocusView();
                    dock->setFloating(true);
                    dock->move(pos - dragOffset);
                    dock->show();
                }
                if (_DragFloating)
                    _DragFloating->hide();
            } else if (!dock->isFloating()) {
                if (!_DragFloating) {
                    _DragFloating = new QDockWidget(getMainWindow());
                    _DragFloating->setFloating(true);
                }
                _DragFloating->resize(dock->size());
                _DragFloating->setWindowTitle(dock->windowTitle());
                _DragFloating->show();
                _DragFloating->move(pos - dragOffset);
            }
            if (_DragFrame)
                _DragFrame->hide();
            return;

        } else if (!drop && _DragFrame && !_DragFrame->isVisible()) {
            _DragFrame->raise();
            _DragFrame->show();
            if (_DragFloating)
                _DragFloating->hide();
        }

        int insertDock = 0; // 0: tabify, -1: insert before, 1: insert after
        if (!dst && dstDock) {
            switch(dstDockArea) {
            case Qt::LeftDockWidgetArea:
            case Qt::RightDockWidgetArea:
                if (pos.y() < rect.top() + rect.height()/4) {
                    insertDock = -1;
                    rect.setBottom(rect.top() + rect.height()/2);
                }
                else if (pos.y() > rect.bottom() - rect.height()/4) {
                    insertDock = 1;
                    int height = rect.height();
                    rect.setTop(rect.bottom() - height/4);
                    rect.setHeight(height/2);
                }
                break;
            default:
                if (pos.x() < rect.left() + rect.width()/4) {
                    insertDock = -1;
                    rect.setRight(rect.left() + rect.width()/2);
                }
                else if (pos.x() > rect.right() - rect.width()/4) {
                    insertDock = 1;
                    int width = rect.width();
                    rect.setLeft(rect.right() - width/4);
                    rect.setWidth(width/2);
                }
                break;
            }
        }

        if (!drop) {
            if (!_DragFrame)
                _DragFrame = new OverlayDragFrame(getMainWindow());

            rect = QRect(getMainWindow()->mapFromGlobal(rect.topLeft()), rect.size());
            _DragFrame->setGeometry(rect);
            if (!outside && !_DragFrame->isVisible()) {
                _DragFrame->raise();
                _DragFrame->show();
            }
            return;
        }

        if (src && src == dst && dstIndex != -2){
            auto splitter = src->getSplitter();
            if (dstIndex == -1) {
                src->tabBar()->moveTab(srcIndex, 0);
                src->setCurrentIndex(0);
                src->onCurrentChanged(0);
            }
            else if (srcIndex != dstIndex) {
                auto sizes = splitter->sizes();
                src->tabBar()->moveTab(srcIndex, dstIndex);
                splitter->setSizes(sizes);
                src->onSplitterResize(dstIndex);
                src->saveTabs();
            }
            return;
        }

        if (src) {
            _overlayMap.erase(dock);
            src->removeWidget(dock);
        }

        setFocusView();
        if (!dst) {
            if (dstDock) {
                dock->setFloating(false);
                if(insertDock == 0)
                    getMainWindow()->tabifyDockWidget(dstDock, dock);
                else {
                    std::map<int, QDockWidget*> docks;
                    for(auto dockWidget : getMainWindow()->findChildren<QDockWidget*>()) {
                        if (dockWidget == dock
                                || !dockWidget->isVisible()
                                || getMainWindow()->dockWidgetArea(dockWidget) != dstDockArea)
                            continue;
                        auto pos = dockWidget->mapToGlobal(QPoint(0,0));
                        if (dstDockArea == Qt::LeftDockWidgetArea || dstDockArea == Qt::RightDockWidgetArea)
                            docks[pos.y()] = dockWidget;
                        else
                            docks[pos.x()] = dockWidget;
                    }
                    auto it = docks.begin();
                    for (;it != docks.end(); ++it) {
                        if (it->second == dstDock)
                            break;
                    }
                    if (insertDock > 0 && it != docks.end())
                        ++it;
                    for (auto iter = it; iter != docks.end(); ++iter)
                        getMainWindow()->removeDockWidget(iter->second);
                    getMainWindow()->addDockWidget(dstDockArea, dock);
                    dock->show();
                    for (auto iter = it; iter != docks.end(); ++iter) {
                        getMainWindow()->addDockWidget(dstDockArea, iter->second);
                        iter->second->show();
                    }
                }
            }
            else {
                dock->setFloating(true);
                if(_DragFrame)
                    dock->setGeometry(QRect(_DragFrame->mapToGlobal(QPoint()),
                                            _DragFrame->size()));
            }
            dock->show();
        }
        else if (dstIndex == -2) {
            getMainWindow()->addDockWidget(dst->getDockArea(), dock);
            dock->setFloating(false);
        }
        else {
            auto sizes = dst->getSplitter()->sizes();
            for (auto o : _overlayInfos) {
                if (o->tabWidget == dst) {
                    o->addWidget(dock, false);
                    break;
                }
            }
            index = dst->dockWidgetIndex(dock);
            if (index >= 0) {
                if (dstIndex < 0) {
                    dst->tabBar()->moveTab(index, 0);
                    dst->setCurrentIndex(0);
                    dst->onCurrentChanged(0);
                }
                else {
                    dst->tabBar()->moveTab(index, dstIndex);
                    int size = sizes[dstIndex + resizeOffset];
                    if (size) {
                        size /= 2;
                        sizes[dstIndex + resizeOffset] = size;
                    }
                    else
                        size = 50;
                    sizes.insert(dstIndex, size);
                    dst->setCurrentIndex(dstIndex);
                    dst->getSplitter()->setSizes(sizes);
                    dst->onSplitterResize(dstIndex);
                    dst->saveTabs();
                }
                dst->setRevealTime(QTime::currentTime().addMSecs(
                            OverlayParams::getDockOverlayRevealDelay()));
            }
        }

        refresh();
    }

    void raiseAll()
    {
        if (raising)
            return;
        Base::StateLocker guard(raising);
        for (OverlayTabWidget *tabWidget : _Overlays) {
            if (tabWidget->isVisible())
                tabWidget->raise();
        }
    }

    void registerDockWidget(const QString &name, OverlayTabWidget *widget) {
        if (name.size())
            _dockWidgetNameMap[name] = widget;
    }

    void unregisterDockWidget(const QString &name, OverlayTabWidget *widget) {
        auto it = _dockWidgetNameMap.find(name);
        if (it != _dockWidgetNameMap.end() && it->second == widget) 
            _dockWidgetNameMap.erase(it);
    }

    void reload(OverlayManager::ReloadMode mode) {
        if (mode == OverlayManager::ReloadResume)
            curReloadMode = mode = OverlayManager::ReloadPending;
        if (mode == OverlayManager::ReloadPending) {
            if (curReloadMode != OverlayManager::ReloadPause) {
                FC_LOG("reload pending");
                _reloadTimer.start(100);
            }
        }
        curReloadMode = mode;
        if (mode == OverlayManager::ReloadPause) {
            FC_LOG("reload paused");
            _reloadTimer.stop();
        }
    }
};


static OverlayManager * _instance;

OverlayManager* OverlayManager::instance()
{
    if ( _instance == 0 )
        _instance = new OverlayManager;
    return _instance;
}

void OverlayManager::destruct()
{
    delete _instance;
    _instance = 0;
}

OverlayManager::OverlayManager()
{
    d = new Private(this, getMainWindow());
    qApp->installEventFilter(this);
}

OverlayManager::~OverlayManager()
{
    delete d;
}

void OverlayManager::setOverlayMode(OverlayMode mode)
{
    d->setOverlayMode(mode);
}


void OverlayManager::initDockWidget(QDockWidget *dw, QWidget *widget)
{
    connect(dw->toggleViewAction(), SIGNAL(triggered(bool)), this, SLOT(onToggleDockWidget(bool)));
    connect(dw, SIGNAL(visibilityChanged(bool)), this, SLOT(onDockVisibleChange(bool)));
    connect(widget, SIGNAL(windowTitleChanged(QString)), this, SLOT(onDockWidgetTitleChange(QString)));

    QString name = dw->objectName();
    if (name.size()) {
        auto it = d->_dockWidgetNameMap.find(dw->objectName());
        if (it != d->_dockWidgetNameMap.end()) {
            for (auto o : d->_overlayInfos) {
                if (o->tabWidget == it->second) {
                    o->addWidget(dw, true);
                    d->onToggleDockWidget(dw, 3);
                    break;
                }
            }
            d->refresh();
        }
    }
}

void OverlayManager::setupDockWidget(QDockWidget *dw, int dockArea)
{
    (void)dockArea;
    d->setupTitleBar(dw);
}

void OverlayManager::unsetupDockWidget(QDockWidget *dw)
{
    d->toggleOverlay(dw, OverlayUnset);
}

void OverlayManager::onToggleDockWidget(bool checked)
{
    auto action = qobject_cast<QAction*>(sender());
    if(!action)
        return;
    d->onToggleDockWidget(qobject_cast<QDockWidget*>(action->parent()), checked);
}

void OverlayManager::onDockVisibleChange(bool visible)
{
    auto dock = qobject_cast<QDockWidget*>(sender());
    if(!dock)
        return;
    FC_TRACE("dock " << dock->objectName().toUtf8().constData()
            << " visible change " << visible << ", " << dock->isVisible());
}

void OverlayManager::onTaskViewUpdate()
{
    auto taskview = qobject_cast<TaskView::TaskView*>(sender());
    if (!taskview)
        return;
    QDockWidget *dock = nullptr;
    DockWnd::ComboView *comboview = nullptr;
    for (QWidget *w=taskview; w; w=w->parentWidget()) {
        if ((dock = qobject_cast<QDockWidget*>(w)))
            break;
        if (!comboview)
            comboview = qobject_cast<DockWnd::ComboView*>(w);
    }
    if (dock) {
        if (comboview && comboview->hasTreeView()) {
            refresh();
            return;
        }
        auto it = d->_overlayMap.find(dock);
        if (it == d->_overlayMap.end()
                || it->second->tabWidget->count() < 2
                || it->second->tabWidget->getAutoMode() != OverlayTabWidget::TaskShow)
            return;
        d->onToggleDockWidget(dock, taskview->isEmpty() ? -2 : 2);
    }
}

void OverlayManager::onDockWidgetTitleChange(const QString &title)
{
    if (title.isEmpty())
        return;
    auto widget = qobject_cast<QWidget*>(sender());
    QDockWidget *dock = nullptr;
    for (QWidget *w=widget; w; w=w->parentWidget()) {
        if ((dock = qobject_cast<QDockWidget*>(w)))
            break;
    }
    if(!dock)
        return;
    auto tabWidget = findTabWidget(dock);
    if (!tabWidget)
        return;
    int index = tabWidget->dockWidgetIndex(dock);
    if (index >= 0)
        tabWidget->setTabText(index, title);
}

void OverlayManager::retranslate()
{
    d->retranslate();
}

void OverlayManager::refreshIcons()
{
    d->refreshIcons();
}

void OverlayManager::onTimer()
{
    d->onTimer();
}

void OverlayManager::reload(ReloadMode mode)
{
    d->reload(mode);
}

void OverlayManager::raiseAll()
{
    d->raiseAll();
}

bool OverlayManager::eventFilter(QObject *o, QEvent *ev)
{
    if (d->intercepting || !getMainWindow() || !o->isWidgetType())
        return false;
    auto mdi = getMainWindow()->getMdiArea();
    if (!mdi)
        return false;

    switch(ev->type()) {
    case QEvent::Enter:
        if (Selection().hasPreselection()
                && !qobject_cast<View3DInventorViewer*>(o)
                && !isUnderOverlay())
        {
            Selection().rmvPreselect();
        }
        break;
    case QEvent::ZOrderChange: {
        if(!d->raising && getMainWindow() && o == mdi) {
            // On Windows, for some reason, it will raise mdi window on tab
            // change in any docked widget, which will then obscure any overlay
            // docked widget here.
            for (auto child : getMainWindow()->children()) {
                if (child == mdi || qobject_cast<QDockWidget*>(child)) {
                    QMetaObject::invokeMethod(this, "raiseAll", Qt::QueuedConnection);
                    break;
                }
                if (qobject_cast<OverlayTabWidget*>(child))
                    break;
            }
        }
        break;
    }
    case QEvent::Resize: {
        if(getMainWindow() && o == mdi)
            refresh();
        return false;
    }
    case QEvent::KeyPress: {
        QKeyEvent *ke = static_cast<QKeyEvent*>(ev);
        bool accepted = false;
        if (ke->modifiers() == Qt::NoModifier && ke->key() == Qt::Key_Escape) {
            if (d->mouseTransparent) {
                d->setMouseTransparent(false);
                accepted = true;
            } else if (_Dragging && _Dragging != o) {
                if (auto titleBar = qobject_cast<OverlayTitleBar*>(_Dragging))
                    titleBar->endDrag();
                else if (auto splitHandle = qobject_cast<OverlaySplitterHandle*>(_Dragging))
                    splitHandle->endDrag();
            }
            else if (!_Dragging) {
                for (OverlayTabWidget *tabWidget : _Overlays) {
                    if (tabWidget->onEscape())
                        accepted = true;
                }
            }
        }
        if (accepted) {
            ke->accept();
            return true;
        }
        break;
    }
    case QEvent::Paint:
        if (auto widget = qobject_cast<QWidget*>(o)) {
            // QAbstractItemView optimize redraw using its item delegate's
            // visualRect(). However, if we are using QGraphicsEffects, the
            // effect may touch areas outside of visualRect(), so
            // OverlayTabWidget offers a timer for a delayed redraw.
            widget = qobject_cast<QAbstractItemView*>(widget->parentWidget());
            if(widget) {
                auto tabWidget = findTabWidget(widget, true);
                if (tabWidget)
                    tabWidget->scheduleRepaint();
            }
        }
        break;
    // case QEvent::NativeGesture:
    case QEvent::Wheel:
        if (!OverlayParams::getDockOverlayWheelPassThrough())
            return false;
        // fall through
    case QEvent::ContextMenu: {
        auto cev = static_cast<QContextMenuEvent*>(ev);
        if (cev->reason() != QContextMenuEvent::Mouse)
            return false;
    }   // fall through
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonPress:
    case QEvent::MouseMove: {
        if (_Dragging && _Dragging != o) {
            if (auto titleBar = qobject_cast<OverlayTitleBar*>(_Dragging))
                titleBar->endDrag();
            else if (auto splitHandle = qobject_cast<OverlaySplitterHandle*>(_Dragging))
                splitHandle->endDrag();
        }
        QWidget *grabber = QWidget::mouseGrabber();
        d->lastIntercept = nullptr;
        if (d->mouseTransparent || (grabber && grabber != d->_trackingOverlay))
            return false;
        if (qobject_cast<QAbstractButton*>(o))
            return false;
        if (ev->type() != QEvent::Wheel) {
            if (qobject_cast<OverlayTitleBar*>(o))
                return false;
        } else if (qobject_cast<QScrollBar*>(o))
            return false;

        if (d->_trackingWidget) {
            if(!isTreeViewDragging())
                d->interceptEvent(d->_trackingWidget, ev);
            if(isTreeViewDragging()
                    || ev->type() == QEvent::MouseButtonRelease
                    || QApplication::mouseButtons() == Qt::NoButton)
            {
                d->_trackingWidget = nullptr;
                if (d->_trackingOverlay == grabber
                        && ev->type() == QEvent::MouseButtonRelease)
                {
                    d->_trackingOverlay = nullptr;
                    // Must not release mouse here, because otherwise the event
                    // will find its way to the actual widget under cursor.
                    // Instead, return false here to let OverlayTabWidget::event()
                    // release the mouse.
                    return false;
                }
                if(d->_trackingOverlay && grabber == d->_trackingOverlay)
                    d->_trackingOverlay->releaseMouse();
                d->_trackingOverlay = nullptr;
            }
            // Must return true here to filter the event, otherwise ContextMenu
            // event may be routed to the actual widget. Other types of event
            // probably do not matter.
            return true;
        } else if (ev->type() != QEvent::MouseButtonPress
                && ev->type() != QEvent::MouseButtonDblClick
                && QApplication::mouseButtons() != Qt::NoButton)
            return false;

        if(isTreeViewDragging())
            return false;

        OverlayTabWidget *activeTabWidget = nullptr;
        int hit = 0;
        QPoint pos = QCursor::pos();
        if (OverlayParams::getDockOverlayAutoMouseThrough()
                    && ev->type() != QEvent::Wheel
                    && pos == d->_lastPos)
        {
            hit = 1;
        } else if (ev->type() == QEvent::Wheel
                && !d->wheelDelay.isNull()
                && (isNear(pos, d->wheelPos) || d->wheelDelay > QTime::currentTime()))
        {
            d->wheelDelay = QTime::currentTime().addMSecs(
                    OverlayParams::getDockOverlayWheelDelay());
            d->wheelPos = pos;
            return false;
        } else {
            for(auto widget=qApp->widgetAt(pos); widget ; widget=widget->parentWidget()) {
                int type = widget->windowType();
                if (type != Qt::Widget && type != Qt::Window) {
                    if (type != Qt::SubWindow)
                        hit = -1;
                    break;
                }
                if (ev->type() == QEvent::Wheel) {
                    if (qobject_cast<OverlayTitleBar*>(widget))
                        activeTabWidget = qobject_cast<OverlayTabWidget*>(widget->parentWidget());
                    else if (qobject_cast<OverlaySplitterHandle*>(widget)) {
                        auto parent = widget->parentWidget();
                        if (parent)
                            activeTabWidget = qobject_cast<OverlayTabWidget*>(parent->parentWidget());
                    }
                    if (activeTabWidget)
                        break;
                }
                if (auto tabWidget = qobject_cast<OverlayTabWidget*>(widget)) {
                    if (tabWidget->testAlpha(pos, ev->type() == QEvent::Wheel ? 4 : 1) == 0)
                        activeTabWidget = tabWidget;
                    break;
                }
            }
            if (activeTabWidget) {
                hit = OverlayParams::getDockOverlayAutoMouseThrough();
                d->_lastPos = pos;
            }
        }

        for (OverlayTabWidget *tabWidget : _Overlays) {
            if (tabWidget->getProxyWidget()->hitTest(pos) > 1) {
                if (ev->type() == QEvent::MouseButtonRelease
                        && static_cast<QMouseEvent*>(ev)->button() == Qt::LeftButton)
                {
                    tabWidget->getProxyWidget()->onMousePress();
                    return false;
                }
            }
        }

        if (hit <= 0) {
            d->_lastPos.setX(INT_MAX);
            if (ev->type() == QEvent::Wheel) {
                d->wheelDelay = QTime::currentTime().addMSecs(OverlayParams::getDockOverlayWheelDelay());
                d->wheelPos = pos;
            }
            return false;
        }

        auto hitWidget = mdi->childAt(mdi->mapFromGlobal(pos));
        if (!hitWidget)
            return false;

        if (!activeTabWidget)
            activeTabWidget = findTabWidget(qApp->widgetAt(QCursor::pos()));
        if(!activeTabWidget || !activeTabWidget->isTransparent())
            return false;

        ev->accept();
        d->interceptEvent(hitWidget, ev);
        if (ev->isAccepted() && ev->type() == QEvent::MouseButtonPress) {
            hitWidget->setFocus();
            d->_trackingWidget = hitWidget;
            d->_trackingOverlay = activeTabWidget;
            d->_trackingOverlay->grabMouse();
        }
        return true;
    }
    default:
        break;
    }
    return false;
}

namespace {
class  MouseGrabberGuard {
public:
    MouseGrabberGuard(QWidget *grabber)
    {
        if (grabber && grabber == QWidget::mouseGrabber()) {
            _grabber = grabber;
            _grabber->releaseMouse();
        }
    }
    ~MouseGrabberGuard()
    {
        if (_grabber)
            _grabber->grabMouse();
    }

    QPointer<QWidget> _grabber;
};
}// anonymous namespace

void OverlayManager::Private::interceptEvent(QWidget *widget, QEvent *ev)
{
    Base::StateLocker guard(this->intercepting);
    MouseGrabberGuard grabberGuard(_trackingOverlay);

    lastIntercept = nullptr;
    auto getChildAt = [](QWidget *w, const QPoint &pos) {
        QWidget *res = w;
        for (; w; w = w->childAt(w->mapFromGlobal(pos))) {
            if (qobject_cast<QGraphicsView*>(w))
                return w;
            res = w;
        }
        return res;
    };

    switch(ev->type()) {
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonPress:
    case QEvent::MouseMove:
    case QEvent::MouseButtonDblClick: {
        auto me = static_cast<QMouseEvent*>(ev);
        lastIntercept = getChildAt(widget, me->globalPos());
        QMouseEvent mouseEvent(ev->type(),
                            lastIntercept->mapFromGlobal(me->globalPos()),
                            me->screenPos(),
                            me->button(),
                            me->buttons(),
                            me->modifiers());
        QApplication::sendEvent(lastIntercept, &mouseEvent);
        break;
    }
    case QEvent::Wheel: {
        auto we = static_cast<QWheelEvent*>(ev);
        lastIntercept = getChildAt(widget, we->globalPos());
        for (auto parent=lastIntercept->parentWidget(); parent; parent=parent->parentWidget()) {
            if (qobject_cast<QGraphicsView*>(parent))
                lastIntercept = parent;
        }
#if QT_VERSION >= QT_VERSION_CHECK(5,12,0)
        QWheelEvent wheelEvent(lastIntercept->mapFromGlobal(we->globalPos()),
                               we->globalPos(),
                               we->pixelDelta(),
                               we->angleDelta(),
                               we->buttons(),
                               we->modifiers(),
                               we->phase(),
                               we->inverted(),
                               we->source());
#else
        QWheelEvent wheelEvent(lastIntercept->mapFromGlobal(we->globalPos()),
                               we->globalPos(),
                               we->pixelDelta(),
                               we->angleDelta(),
                               0,
                               Qt::Vertical,
                               we->buttons(),
                               we->modifiers(),
                               we->phase(),
                               we->source(),
                               we->inverted());
#endif
        QApplication::sendEvent(lastIntercept, &wheelEvent);
        break;
    }
    case QEvent::ContextMenu: {
        auto ce = static_cast<QContextMenuEvent*>(ev);
        lastIntercept = getChildAt(widget, ce->globalPos());
        QContextMenuEvent contextMenuEvent(ce->reason(), 
                                           lastIntercept->mapFromGlobal(ce->globalPos()),
                                           ce->globalPos());
        QApplication::sendEvent(lastIntercept, &contextMenuEvent);
        break;
    }
    default:
        break;
    }
}

void OverlayManager::refresh(QWidget *widget, bool refreshStyle)
{
    d->refresh(widget, refreshStyle);
}

void OverlayManager::setMouseTransparent(bool enabled)
{
    d->setMouseTransparent(enabled);
}

bool OverlayManager::isMouseTransparent() const
{
    return d->mouseTransparent;
}

bool OverlayManager::isUnderOverlay() const
{
    return OverlayParams::getDockOverlayAutoMouseThrough()
        && findTabWidget(qApp->widgetAt(QCursor::pos()), true);
}

void OverlayManager::save()
{
    d->save();
}

void OverlayManager::restore()
{
    d->restore();

    if (Control().taskPanel())
        connect(Control().taskPanel(), SIGNAL(taskUpdate()), this, SLOT(onTaskViewUpdate()));
}

void OverlayManager::changeOverlaySize(int changes)
{
    d->changeOverlaySize(changes);
}

void OverlayManager::onFocusChanged(QWidget *old, QWidget *now)
{
    d->onFocusChanged(old, now);
}

void OverlayManager::setupTitleBar(QDockWidget *dock)
{
    d->setupTitleBar(dock);
}

void OverlayManager::onAction()
{
    QAction *action = qobject_cast<QAction*>(sender());
    if(action)
        d->onAction(action);
}

void OverlayManager::dragDockWidget(const QPoint &pos,
                                    QWidget *src,
                                    const QPoint &offset,
                                    const QSize &size,
                                    bool drop)
{
    d->dragDockWidget(pos, src, offset, size, drop);
}

void OverlayManager::floatDockWidget(QDockWidget *dock)
{
    d->floatDockWidget(dock);
}

void OverlayManager::registerDockWidget(const QString &name, OverlayTabWidget *widget)
{
    d->registerDockWidget(name, widget);
}

void OverlayManager::unregisterDockWidget(const QString &name, OverlayTabWidget *widget)
{
    d->unregisterDockWidget(name, widget);
}

bool OverlayManager::isChangingMode() const
{
    return _ChangingOverlayMode;
}

QWidget *OverlayManager::getLastMouseInterceptWidget() const
{
    return d->lastIntercept;
}

#include "moc_OverlayWidgets.cpp"
