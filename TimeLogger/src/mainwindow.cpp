#include <iostream>
#include <fstream>
#include <ctime>
#include <chrono>
#include <vector>
#include <array>

#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QMessageBox>
#include <QRegularExpression>
#include <QScrollBar>

#include "mainwindow.h"
#include "./ui_mainwindow.h"

std::string getCurrentTime() {
  auto now = std::chrono::system_clock::now();
  auto nowT = std::chrono::system_clock::to_time_t(now);
  std::string nowStr = std::ctime(&nowT);
  if (!nowStr.empty() && nowStr.back() == '\n') {
    nowStr.pop_back();
  }
  return nowStr;
}

void MainWindow::on_startButton_clicked() {
  std::ofstream logFile("../log.txt", std::ios_base::app);
  if (!logFile.is_open()) {
    perror("Failed to open log");
    return;
  }
  std::string nowStr = getCurrentTime();

  logFile << '[' << nowStr << "] 'Started'\n";
  logFile.close();
}

void MainWindow::on_stopButton_clicked() {
  std::ofstream logFile("../log.txt", std::ios_base::app);
  if (!logFile.is_open()) {
    perror("Failed to open log");
    return;
  }
  std::string nowStr = getCurrentTime();

  logFile << '[' << nowStr << "] 'Stopped'\n";
  logFile.close();
}

void MainWindow::onScroll(int value) {
  QScrollBar *scrollBar = ui->logTable->verticalScrollBar();
  if (value == scrollBar->maximum()) {
    static int startLine = 0;
    int batchSize = 100;

    int loadedRows = loadLogBatch(startLine, batchSize);
    if (loadedRows > 0) {
        startLine += loadedRows;
    } 
  }
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
  ui->setupUi(this);

  int loadedRows = loadLogBatch(0, 100);
  if (loadedRows < 0) {
    QMessageBox::warning(this, "Error", "Failed to parse batch.");
  }

  connect(ui->logTable->verticalScrollBar(), &QScrollBar::valueChanged, this, &MainWindow::onScroll);
}

MainWindow::~MainWindow()
{
    delete ui;
}
