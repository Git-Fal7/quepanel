#include "qmtaskgroup.h"

#include <KWindowSystem>
#include <NETWM>
#include <QStyle>
#include <QX11Info>
#include <QMenu>
#include <QDebug>
#include <QProcess>

QMTaskGroup::QMTaskGroup(const QString &groupname, QIcon icon, QWidget * parent, bool pinned)
    : QToolButton(parent),
    mPinned(pinned),
    mGroupID(groupname),
    mIcon(icon),
    mList(*new QList<WId>)
{
    setCheckable(true);
    setToolButtonStyle(Qt::ToolButtonIconOnly);
    setAcceptDrops(true);

    setIcon(mIcon);
    connect(this, &QToolButton::clicked, this, &QMTaskGroup::active);
}

void QMTaskGroup::active(){
    if (mList.size() == 0){
        QProcess::startDetached(mGroupID);
        setChecked(false);
    } else {
        if (isChecked()){
            KWindowSystem::forceActiveWindow(mList[0]);
        } else {
            if (isSingle()) {
                KWindowSystem::minimizeWindow(mList[0]);
            } else {
                if (getActiveWid() < mList.size() - 1) {
                    KWindowSystem::forceActiveWindow(mList[getActiveWid() + 1]);
                    setChecked(true);
                } else {
                    for (int i = 0; i < mList.size(); i++){
                        KWindowSystem::minimizeWindow(mList[i]);
                    }
                }
            }
        }
    }
}

void QMTaskGroup::addWindow(WId window){
    if (mList.contains(window))
        return;
    if (isActive(window))
        setChecked(true);
    mList << window;
}

void QMTaskGroup::removeWindow(WId window){
    if (!mList.contains(window))
        return;
    mList.removeAt(mList.indexOf(window));
}

void QMTaskGroup::closeGroup()
{
    for (int i = 0; i < mList.size(); i++){
        NETRootInfo(QX11Info::connection(), NET::CloseWindow).closeWindowRequest(mList[i]);
    }
}

QSize QMTaskGroup::sizeHint() const
{
    return {QToolButton::sizeHint().width() + 5, QToolButton::sizeHint().height()};
}

//Create context menu for the window list.
//Will be changed.
void QMTaskGroup::contextMenuEvent(QContextMenuEvent *event){
    QMenu * menu = new QMenu(tr("Application"));
    menu->setAttribute(Qt::WA_DeleteOnClose);
    QAction *a;
    for (int i = 0; i < mList.size(); i++){
        KWindowInfo info(mList[i], NET::WMVisibleName | NET::WMName);
        QString title = info.visibleName();
        if (title.isEmpty())
            title = info.name();
        a = menu->addAction(KWindowSystem::icon(mList[i]), title.replace("&", "&&"));
        if(isActive(mList[i])){
            QFont font = a->font();
            font.setBold(true);
            a->setFont(font);
        }
        a->setWhatsThis(QString::number(i));
        //connect(a, &QAction::triggered, a, &QMTaskGroup::activeWindow(i));
    }
    //connect(menu, SIGNAL(triggered(QAction*)), SLOT(QMTaskGroup::menuAction(QAction*)));
    menu->show();
}

void QMTaskGroup::menuAction(QAction *action){
    KWindowSystem::forceActiveWindow(mList[action->whatsThis().toInt()]);
}

void QMTaskGroup::activeWindow(int num){
    KWindowSystem::forceActiveWindow(mList[num]);
}

bool QMTaskGroup::isSingle() const{
    return mList.size() < 2;
}

int QMTaskGroup::getActiveWid(){
    return mList.indexOf(KWindowSystem::activeWindow());
}

//Taken from lxqt
bool QMTaskGroup::isActive(WId window) {
    return KWindowSystem::activeWindow() == window;
}

//Unused.
bool QMTaskGroup::isHidden(WId window) {
    KWindowInfo info(window, NET::WMState);
    return (info.state() & NET::Hidden);
}

bool QMTaskGroup::isPinned() const{
    return mPinned;
}
