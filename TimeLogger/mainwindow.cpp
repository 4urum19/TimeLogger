#include <iostream>
#include <fstream>
#include <ctime>
#include <chrono>
#include <vector>

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

QStringList readLines(QFile &file, const int startLine, const int batchSize) {
  QStringList lines;
  QString line;

  int currentPos = file.pos();
  int lineCount = 0;

  while (currentPos > 0 && lineCount < startLine + batchSize) {
    currentPos -= 1;
    file.seek(currentPos);
    char c;
    file.read(&c, 1);

    if (c == '\n') {
      if (lineCount >= startLine) {
        file.seek(currentPos + 1);
        line = file.readLine();
        lines.append(line);
      }
      lineCount += 1;
    }
  }

  if (currentPos == 0) {
    file.seek(currentPos);
    line = file.readLine();
    lines.append(line);
  }

  return lines;
}

int MainWindow::loadLogBatch(const int startLine, const int batchSize) {
  QFile file("../log.txt");
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    QMessageBox::warning(this, "Error", "Failed to open log file.");
    return -1;
  }

  QTextStream in(&file);
  file.seek(file.size());

  QStringList lines = readLines(file, startLine, batchSize);

  file.close();

  if (lines.isEmpty()) {
    return 0;
  }

  QString lastTimeStamp;
  QRegularExpression reg(R"(\[(.*)\] '(.*)')");
  QRegularExpressionMatch match;

  for (const QString &line : lines) {
    match = reg.match(line);
    if (match.hasMatch()) {
      QString timeStamp = match.captured(1);
      QString commitMsg = match.captured(2);

      std::cerr << timeStamp.toStdString() << ' ' << commitMsg.toStdString() << '\n';

      int row = ui->logTable->rowCount();
      ui->logTable->insertRow(row);
      ui->logTable->setItem(row, 0, new QTableWidgetItem(timeStamp));

      if (!lastTimeStamp.isEmpty()) {
        QDateTime lastTime = QDateTime::fromString(lastTimeStamp, "ddd MMM d HH:mm:ss yyyy");
        QDateTime newTime = QDateTime::fromString(timeStamp, "ddd MMM d HH:mm:ss yyyy");

        if (lastTime.isValid() && newTime.isValid()) {
          qint64 secondsDiff = newTime.secsTo(lastTime);
          qint64 hours = secondsDiff / 3600;
          qint64 minutes = (secondsDiff % 3600) / 60;
          qint64 seconds = secondsDiff % 60;

          QString timeDiff = QString("%1:%2:%3")
            .arg(hours, 2, 10, QChar('0'))   
            .arg(minutes, 2, 10, QChar('0')) 
            .arg(seconds, 2, 10, QChar('0')); 

          ui->logTable->setItem(row, 1, new QTableWidgetItem(timeDiff));
        }
      }

      ui->logTable->setItem(row, 2, new QTableWidgetItem(commitMsg));
    
      lastTimeStamp = timeStamp;
      row += 1;
    }
  }

  return lines.size();
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
