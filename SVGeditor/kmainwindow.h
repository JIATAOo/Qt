#ifndef __K_MAINWINDOW_H_
#define __K_MAINWINDOW_H_

#include <QtWidgets/QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "ksvgmainwindow.h"
#include "ktoolbar.h"
#include "kcanvasparamsbar.h"

#include "ui_kmainwindow.h"

class KMainWindow : public QWidget
{
    Q_OBJECT

public:
    KMainWindow(QWidget *parent = Q_NULLPTR);
    void initWindows();
    void initConnection();
  

private slots:
    void validateCanvasParams();

private:
    Ui::KMainWindowClass ui;
    QVBoxLayout *m_pVLayout;
    QHBoxLayout *m_pHLayout;
    KToolBar *m_pToolBar;
    KSvgMainWindow *m_pSvgMainWin;
    KCanvasParamsBar *m_pCanvasParamsBar;
};

#endif
