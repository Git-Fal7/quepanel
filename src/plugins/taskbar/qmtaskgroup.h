#ifndef QMTASKGROUP_H
#define QMTASKGROUP_H

#include <QToolButton>

class QAbstractButton;
class QMTaskGroup : public QToolButton
{
public:
    QMTaskGroup(const QString &groupname, QIcon icon, QWidget * parent, bool pinned);
    QSize sizeHint() const override;
    void addWindow(WId window);
    void removeWindow(WId window);
    bool isSingle() const;
    bool isPinned() const;
public slots:
        void active();
        void closeGroup();
    void menuAction(QAction *action);
protected:
    /*void dragEnterEvent(QDragEnterEvent * event) override;
    void dragLeaveEvent(QDragLeaveEvent * event) override;
    void dropEvent(QDropEvent * event) override;
    void mousePressEvent(QMouseEvent * event) override;*/
    void contextMenuEvent(QContextMenuEvent *event) override;
private:
    QString mGroupID;
    QIcon mIcon;
    bool mPinned;
    QList<WId> mList;
    bool isActive(WId window);
    bool isHidden(WId window);
    void activeWindow(int num);
    int getActiveWid();
};

#endif // QMTASKBUTTON_H
