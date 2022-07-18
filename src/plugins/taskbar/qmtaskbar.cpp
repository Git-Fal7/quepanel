#include "qmtaskbar.h"
#include "qmtaskgroup.h"
#include "../../resources.h"

#include <KWindowSystem>
#include <NETWM>
#include <QX11Info>
#include <QDebug>
#include <QStyle>

QMTaskBar::QMTaskBar(Resources & res, QWidget * parent)
    : QWidget(parent),
    mLayout(this)
{
    mLayout.setContentsMargins(QMargins());
    mLayout.setSpacing(0);
    mLayout.addStretch(1);

    setAcceptDrops(true);
    for (auto app : res.settings().quickLaunchApps){
        if (!res.getInfo(app))
            continue;
        auto list = res.getInfo(app)[0];
        auto group = new QMTaskGroup(list[1], QIcon::fromTheme(list[0]), this, true);
        mLayout.insertWidget(mLayout.count() - 1, group);
        mHashGroups.insert(list[1], group);
        qDebug() << list[1];
    }
    for (auto window : KWindowSystem::stackingOrder())
    {
        if (acceptWindow(window))
            addWindow(window);
    }

    connect(KWindowSystem::self(), &KWindowSystem::windowAdded, this,
            &QMTaskBar::addWindow);
    connect(KWindowSystem::self(), &KWindowSystem::windowRemoved, this, &QMTaskBar::removeWindow);
    connect(KWindowSystem::self(), &KWindowSystem::activeWindowChanged, this,&QMTaskBar::onActiveWindowChanged);
}

bool QMTaskBar::acceptWindow(WId window) const
{
    const NET::WindowTypes ignoreList =
        NET::DesktopMask | NET::DockMask | NET::SplashMask | NET::ToolbarMask |
        NET::MenuMask | NET::PopupMenuMask | NET::NotificationMask;

    KWindowInfo info(window, NET::WMWindowType | NET::WMState,
                     NET::WM2TransientFor);

    if (!info.valid() ||
        NET::typeMatchesMask(info.windowType(NET::AllTypesMask), ignoreList) ||
        (info.state() & NET::SkipTaskbar))
    {
        return false;
    }

    WId transFor = info.transientFor();
    if (transFor == 0 || transFor == window ||
        transFor == (WId)QX11Info::appRootWindow())
    {
        return true;
    }

    return false;
}

void QMTaskBar::addWindow(WId window)
{
    const QString group_id = QString::fromUtf8(KWindowInfo(window, NET::Properties(), NET::WM2WindowClass).windowClassClass()).toLower();
    if (group_id != "qmpanel"){
        QMTaskGroup* group;
        if (mHashGroups.value(group_id) == NULL){
            int size = style()->pixelMetric(QStyle::PM_ToolBarIconSize);
            size *= devicePixelRatioF();

            group = new QMTaskGroup(group_id, KWindowSystem::icon(window, size, size), this, false);

            mLayout.insertWidget(mLayout.count() - 1, group);

            mHashWindows.insert(window, group_id);
            mHashGroups.insert(group_id, group);
        } else {
            mHashWindows.insert(window, group_id);
            group = mHashGroups.value(group_id);
        }
        //Look qmtaskgroup.cpp
        group->addWindow(window);
    }

}

void QMTaskBar::removeWindow(WId window)
{
    QString group_id = mHashWindows.value(window);
    QMTaskGroup* group = mHashGroups.value(group_id);
    if (group->isSingle()){
        if (!group->isPinned()){
            mLayout.removeWidget(group);
            mHashGroups.remove(group_id);
            group->deleteLater();
        } else {
            group->setChecked(false);
        }
    }
    group->removeWindow(window);
    mHashWindows.remove(window);
}

void QMTaskBar::onActiveWindowChanged(WId window)
{
    //Just iterates to the correct group, if there isn't (like clicking on the desktop), it will uncheck all
    QHashIterator<QString, QMTaskGroup *> it = mHashGroups;
    while (it.hasNext()){
        it.next();
        if (it.key() == mHashWindows.value(window)){
            it.value()->setChecked(true);
        } else {
            it.value()->setChecked(false);
        }
    }
}

//Pin group
//void QMTaskBar::pinGroup(QMTaskGroup group){

//}
