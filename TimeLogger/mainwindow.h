#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

public slots:
  void on_startButton_clicked();
  void on_stopButton_clicked();

private:
  int loadLogBatch(
    const int startLine, 
    const int batchSize
  );
  void onScroll(
    int value
  );    

  Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
