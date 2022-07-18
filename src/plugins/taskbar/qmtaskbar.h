#ifndef QMTASKBAR_H
#define QMTASKBAR_H

#include <NETWM>
#include <QHBoxLayout>
#include <QWidget>
#include "../../resources.h"

class QMTaskGroup;

class QMTaskBar : public QWidget
{
public:
    explicit QMTaskBar(Resources & res,QWidget * parent);

private:
    bool acceptWindow(WId window) const;
    void addWindow(WId window);
    void removeWindow(WId window);
    void onWindowAdded(WId window);
    void onActiveWindowChanged(WId window);
    void onWindowChanged(WId window, NET::Properties prop,
                         NET::Properties2 prop2);
    QHash<WId, QString> mHashWindows;
    QHash<QString, QMTaskGroup *> mHashGroups;
    QHBoxLayout mLayout;
};

#endif
