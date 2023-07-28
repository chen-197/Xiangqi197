#include "mainwindow.h"
#include "ui_mainwindow.h"

int clickedButtonNum = -1;
QPoint mousePoint;
QPoint relaPoint;
QPoint beginButtonPoint;
QPoint qipanCoordinates[9][10] = {};
QPoint qiziCoordinate[32] = {QPoint(0, 0), QPoint(0, 632), QPoint(0, 79), QPoint(0, 553), QPoint(0, 158), QPoint(0, 474), QPoint(0, 237), QPoint(0, 395), QPoint(0, 316), QPoint(272, 79), QPoint(272, 553), QPoint(408, 0), QPoint(408, 158), QPoint(408, 316), QPoint(408, 474), QPoint(408, 632), QPoint(1233, 0), QPoint(1233, 632), QPoint(1233, 79), QPoint(1233, 553), QPoint(1233, 158), QPoint(1233, 474), QPoint(1233, 237), QPoint(1233, 395), QPoint(1233, 316), QPoint(961, 79), QPoint(961, 553), QPoint(825, 0), QPoint(825, 158), QPoint(825, 316), QPoint(825, 474), QPoint(825, 632)};
QString qiziIsChuOrHan[32];
QString chuhanRound = "none";
QString PrechuhanRound = "none";
QString ifOver = "none";
QString FEN;
QString NetBinPath;
fs::path saveDir = "Saved";
QPushButton *allButton[32];
QPushButton *chButton = nullptr;
QTimer autoTimer;
QProcess myProcess;
preStep yiqvshiStep;
preStep huiqiStep;
preStep cStep;
int autoNum = 0;
int autoSum = 0;
int distance[4];
int comboIndex = 0;
bool ifPressed = false;
bool Pause = false;
bool ifReadyRead = false;

bool CheckRoundCorrectAndSetRound()
{
    PrechuhanRound = chuhanRound;
    int qiziNum = -1;
    for (int i = 0; i < 32; i++)
    {
        if (chButton == allButton[i])
        {
            qiziNum = i;
            break;
        }
    }
    if (chuhanRound == "none")
    {
        if (qiziIsChuOrHan[qiziNum] == "Chu")
        {
            chuhanRound = "Han";
        }
        else
        {
            chuhanRound = "Chu";
        }
        return true;
    }
    if (qiziIsChuOrHan[qiziNum] == "Chu")
    {
        if (chuhanRound == "Han")
        {
            QMessageBox roundBox;
            roundBox.setText("轮到红方出棋。");
            roundBox.exec();
            ifPressed = false;
            return false;
        }
        else
        {
            chuhanRound = "Han";
            return true;
        }
    }
    if (qiziIsChuOrHan[qiziNum] == "Han")
    {
        if (chuhanRound == "Chu")
        {
            QMessageBox roundBox;
            roundBox.setText("轮到黑方出棋。");
            roundBox.exec();
            ifPressed = false;
            return false;
        }
        else
        {
            chuhanRound = "Chu";
            return true;
        }
    }
    return true;
}
void ShowfBoxAndQuash(QString text)
{
    autoTimer.stop();
    ifPressed = false;
    chuhanRound = PrechuhanRound;
    if (cStep.preButton == nullptr)
    {
        qDebug() << "ERROR!";
        return;
    }
    QMessageBox fBox;
    fBox.setText(text);
    fBox.exec();
    cStep.preButton->move(cStep.prePoint);
    cStep = preStep();
    autoTimer.start(1000);
}
char qiziCharac(QPushButton *quButton)
{
    if (quButton == allButton[0] || quButton == allButton[1])
    {
        return 'r';
    }
    else if (quButton == allButton[2] || quButton == allButton[3])
    {
        return 'n';
    }
    else if (quButton == allButton[4] || quButton == allButton[5])
    {
        return 'b';
    }
    else if (quButton == allButton[6] || quButton == allButton[7])
    {
        return 'a';
    }
    else if (quButton == allButton[8])
    {
        return 'k';
    }
    else if (quButton == allButton[9] || quButton == allButton[10])
    {
        return 'c';
    }
    else if (quButton == allButton[11] || quButton == allButton[12] || quButton == allButton[13] || quButton == allButton[14] || quButton == allButton[15])
    {
        return 'p';
    }
    else if (quButton == allButton[16] || quButton == allButton[17])
    {
        return 'R';
    }
    else if (quButton == allButton[18] || quButton == allButton[19])
    {
        return 'N';
    }
    else if (quButton == allButton[20] || quButton == allButton[21])
    {
        return 'B';
    }
    else if (quButton == allButton[22] || quButton == allButton[23])
    {
        return 'A';
    }
    else if (quButton == allButton[24])
    {
        return 'K';
    }
    else if (quButton == allButton[25] || quButton == allButton[26])
    {
        return 'C';
    }
    else if (quButton == allButton[27] || quButton == allButton[28] || quButton == allButton[29] || quButton == allButton[30] || quButton == allButton[31])
    {
        return 'P';
    }
    return 'E';
}
QString getFEN()
{
    QString FEN_TEMP;
    for (int i = 0; i < 10; i++)
    {
        int spacenum = 0;
        for (int s = 0; s < 9; s++)
        {
            bool ifexqizi = false;
            int ex = 0;
            for (int k = 0; k < 32; k++)
            {
                if (QPoint(allButton[k]->x(), allButton[k]->y()) == qipanCoordinates[s][i])
                {
                    ifexqizi = true;
                    ex = k;
                    break;
                }
            }
            if (ifexqizi)
            {
                if (spacenum != 0)
                {
                    FEN_TEMP += QString::number(spacenum);
                    spacenum = 0;
                }
                FEN_TEMP += qiziCharac(allButton[ex]);
            }
            else
            {
                spacenum++;
            }
            if (s == 8 && spacenum != 0)
            {
                FEN_TEMP += QString::number(spacenum);
            }
        }
        if (i != 9)
        {
            FEN_TEMP += "/";
        }
    }
    if (chuhanRound == "Chu")
    {
        FEN_TEMP += " b";
    }
    else
    {
        FEN_TEMP += " w";
    }
    qDebug() << FEN_TEMP;
    return FEN_TEMP;
}
int charToqipanInt(char charK)
{
    char abc[] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i'};
    for (int i = 0; i < sizeof(abc) / sizeof(abc[0]); i++)
    {
        if (abc[i] == charK)
        {
            return i;
        }
    }
    return -1;
}
void MainWindow::analysisStep(std::string stepStr)
{
    qDebug() << QString::fromStdString(stepStr);
    int charOne = charToqipanInt(stepStr[5]);
    int charTwo = charToqipanInt(stepStr[7]);
    int kq = -1;
    std::stringstream stream;
    stream << stepStr[6];
    int intOne;
    stream >> intOne;
    stream.clear();
    intOne = 9 - intOne;
    stream << stepStr[8];
    int intTwo;
    stream >> intTwo;
    stream.clear();
    intTwo = 9 - intTwo;
    qDebug() << "YES1" << charOne << intOne;
    for (int k = 0; k < 32; k++)
    {
        if (QPoint(allButton[k]->x(), allButton[k]->y()) == qipanCoordinates[charOne][intOne])
        {
            kq = k;
            break;
        }
    }
    if (kq == -1)
    {
        return;
    }
    chButton = allButton[kq];
    chButton->raise();
    cStep = preStep(chButton, QPoint(chButton->x(), chButton->y()));
    if (kq < 16)
    {
        chuhanRound = "Han";
    }
    else
    {
        chuhanRound = "Chu";
    }
    int rx = intTwo;
    int ry = charTwo;
    bool ifexate = false;
    for (int i = 0; i < 32; i++)
    {
        if (QPoint(allButton[i]->x(), allButton[i]->y()) == qipanCoordinates[ry][rx] && allButton[i] != chButton)
        {
            ifexate = true;
            yiqvshiStep = preStep(allButton[i], QPoint(allButton[i]->x(), allButton[i]->y()));
            allButton[i]->move(-114, -514);
            if (allButton[i] == ui->Chu_Jiang)
            {
                ifOver = "Chu";
            }
            else if (allButton[i] == ui->Han_Jiang)
            {
                ifOver = "Han";
            }
            break;
        }
    }
    chButton->move(qipanCoordinates[ry][rx]);
    if (!ifexate)
    {
        yiqvshiStep = preStep();
    }
    if (ifOver == "none")
    {
        huiqiStep = cStep;
    }
    else
    {
        huiqiStep = preStep();
        yiqvshiStep = preStep();
    }
    if (ifOver == "Chu")
    {
        QMessageBox::StandardButton result = QMessageBox::information(NULL, "胜负已分！", "恭喜红方获胜！\n是否要重置对局？", QMessageBox::Yes | QMessageBox::No);
        switch (result)
        {
        case QMessageBox::Yes:
            for (int i = 0; i < 32; i++)
            {
                allButton[i]->move(qiziCoordinate[i]);
            }
            ifOver = "none";
            chuhanRound = "none";
            PrechuhanRound = "none";
            MainWindow::on_Continue_clicked();
            break;
        default:
            break;
        }
    }
    else if (ifOver == "Han")
    {
        QMessageBox::StandardButton result = QMessageBox::information(NULL, "胜负已分！", "恭喜黑方获胜！\n是否要重置对局？", QMessageBox::Yes | QMessageBox::No);
        switch (result)
        {
        case QMessageBox::Yes:
            for (int i = 0; i < 32; i++)
            {
                allButton[i]->move(qiziCoordinate[i]);
            }
            ifOver = "none";
            chuhanRound = "none";
            PrechuhanRound = "none";
            MainWindow::on_Continue_clicked();
            break;
        default:
            break;
        }
    }
    chButton = nullptr;
}
void MainWindow::on_readoutput()
{
    ifReadyRead = true;
    qDebug() << "READY!";
}
void MainWindow::xiangqitimeEvent()
{
    if (autoNum == 0)
    {
        autoTimer.stop();
        return;
    }
    if (autoNum == -1 && chuhanRound != "Chu")
    {
        return;
    }
    if (autoNum == 1 && chuhanRound != "Han")
    {
        return;
    }
    if (autoNum == 2 && chuhanRound == "none")
    {
        chuhanRound = "Chu";
        qDebug() << "准备观摩一场视觉盛宴吧！";
    }
    if (autoSum == 1)
    {
        autoTimer.stop();
        int difficulty = 9;
        if (ui->pve_radioButton->isChecked())
        {
            difficulty = 7 - comboIndex * 2;
        }
        if (ui->eve_radioButton->isChecked())
        {
            difficulty = 9 - comboIndex / 2 * 4;
        }
        if (ui->actionLocal->isChecked())
        {
            std::ofstream fout;
            fout.open("query.py", std::ios::out);
            fout << "import requests" << std::endl;
            fout << "import random" << std::endl;
            fout << "try:" << std::endl;
            fout << "    header={'User-Agent':'Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/105.0.0.0 Safari/537.36','Referer': 'http://www.chessdb.cn/',}" << std::endl;
            fout << "    link = \"http://www.chessdb.cn/chessdb.php?action=queryall&board=" << getFEN().toStdString() << "&showall=1\"" << std::endl;
            fout << "    r = requests.get(link, headers=header)" << std::endl;
            fout << "    html = r.content" << std::endl;
            fout << "    html = html.decode(\"utf-8\")" << std::endl;
            fout << "    abc = html.split('|')" << std::endl;
            fout << "    k = random.randint(1,2)" << std::endl;
            fout << "    if(k == 1):" << std::endl;
            fout << "        print(abc[0])" << std::endl;
            fout << "    elif(len(abc) > " << difficulty << "):" << std::endl;
            fout << "        print(abc[random.randint(1," << difficulty << ")])" << std::endl;
            fout << "    else:" << std::endl;
            fout << "        print(abc[random.randint(0,len(abc) - 1)])" << std::endl;
            fout << "except:" << std::endl;
            fout << "    print(\"ERROR\")" << std::endl;
            fout.close();
            qDebug() << "本次走棋难度系数：" << difficulty;
            QStringList strlist;
            strlist << "query.py";
            myProcess.start("python", strlist);
        }
        else if (ui->actionLocal_bin->isChecked())
        {
            if (NetBinPath.isEmpty())
            {
                autoSum = 0;
                qDebug() << "ERROR BIN PATH";
                ui->label_3->setText("ERROR BIN PATH!!!");
                on_Pause_clicked();
                return;
            }
            QStringList tmpList;
            tmpList << QString("\"" + getFEN() + "\"") << QString::number(difficulty);
            myProcess.start(NetBinPath, tmpList);
        }
        else
        {
            autoSum = 0;
            qDebug() << "ERROR START QUE!!!";
            ui->label_3->setText("ERROR START QUE!!!");
            on_Pause_clicked();
            return;
        }
        autoTimer.start(1000);
    }
    if (ifReadyRead)
    {
        autoTimer.stop();
        ifReadyRead = false;
        autoSum = 0;
        QString readInfo = myProcess.readAll();
        ui->label_3->setText("已成功连接至中国象棋云库！");
        if (readInfo[0] == 'm' && readInfo[1] == 'o' && readInfo[2] == 'v')
        {
            analysisStep(readInfo.toStdString());
            autoTimer.start(1000);
        }
        else if (readInfo[0] == 'E')
        {
            autoTimer.stop();
            ui->pvp_radioButton->setChecked(true);
            MainWindow::on_pvp_radioButton_clicked();
            autoNum = 0;
            ui->label_3->setText("1型网络异常！");
            QMessageBox box;
            box.setText("Network Error!");
            box.exec();
        }
        else if (readInfo[0] == 'i' && readInfo[1] == 'n')
        {
            autoTimer.stop();
            ui->pvp_radioButton->setChecked(true);
            MainWindow::on_pvp_radioButton_clicked();
            autoNum = 0;
            QMessageBox box;
            box.setText("下的什么烂棋？和别人玩去吧。");
            box.exec();
        }
        else if ((readInfo[0] == 'u' && readInfo[1] == 'n') || (readInfo[0] == 'n' && readInfo[1] == 'o'))
        {
            autoTimer.stop();
            ui->pvp_radioButton->setChecked(true);
            MainWindow::on_pvp_radioButton_clicked();
            autoNum = 0;
            QMessageBox box;
            box.setText("你成功把电脑给整不会了，电脑不和你玩了。");
            box.exec();
        }
        else if ((readInfo[0] == 'c' && readInfo[1] == 'h') || (readInfo[0] == 's'))
        {
            autoTimer.stop();
            ui->pvp_radioButton->setChecked(true);
            MainWindow::on_pvp_radioButton_clicked();
            autoNum = 0;
            QMessageBox box;
            box.setText("胜负已分！");
            box.exec();
        }
        else
        {
            autoTimer.stop();
            ui->pvp_radioButton->setChecked(true);
            MainWindow::on_pvp_radioButton_clicked();
            autoNum = 0;
            QMessageBox box;
            box.setText("出现错误，可能是因必要的组件缺失而导致。");
            box.exec();
        }
    }
    if (!ifReadyRead && autoSum > 16)
    {
        autoTimer.stop();
        ui->pvp_radioButton->setChecked(true);
        MainWindow::on_pvp_radioButton_clicked();
        autoSum = 0; // 这一行其实没啥用，因为上面的函数调用已经有相关部分了，写出来就当提示一下此处应该有该部分吧。
        autoNum = 0;
        ui->label_3->setText("2型网络异常！");
        ifReadyRead = false;
        QMessageBox box;
        box.setText("Network Error!");
        box.exec();
    }
    autoSum++;
}
int absoluteValue(int ab)
{
    if (ab <= 0)
    {
        return -ab;
    }
    else
    {
        return ab;
    }
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    /*
    QImage bgimg;
    bgimg.load(QString(":/Img/Img/Qipan.png"));
    ui->Qipan->setPixmap(QPixmap::fromImage(bgimg));
    */
    for (int i = 0; i < 9; i++)
    {
        for (int s = 0; s < 10; s++)
        {
            if (s <= 4)
            {
                qipanCoordinates[i][s] = QPoint(s * 136, i * 79);
                qDebug() << qipanCoordinates[i][s];
            }
            else
            {
                qipanCoordinates[i][s] = QPoint(1233 - 136 * (9 - s), i * 79);
                qDebug() << qipanCoordinates[i][s];
            }
        }
    }
    allButton[0] = ui->Chu_Jv1;
    allButton[1] = ui->Chu_Jv2;
    allButton[2] = ui->Chu_Ma1;
    allButton[3] = ui->Chu_Ma2;
    allButton[4] = ui->Chu_Xiang1;
    allButton[5] = ui->Chu_Xiang2;
    allButton[6] = ui->Chu_Shi1;
    allButton[7] = ui->Chu_Shi2;
    allButton[8] = ui->Chu_Jiang;
    allButton[9] = ui->Chu_Pao1;
    allButton[10] = ui->Chu_Pao2;
    allButton[11] = ui->Chu_Bing1;
    allButton[12] = ui->Chu_Bing2;
    allButton[13] = ui->Chu_Bing3;
    allButton[14] = ui->Chu_Bing4;
    allButton[15] = ui->Chu_Bing5;
    allButton[16] = ui->Han_Jv1;
    allButton[17] = ui->Han_Jv2;
    allButton[18] = ui->Han_Ma1;
    allButton[19] = ui->Han_Ma2;
    allButton[20] = ui->Han_Xiang1;
    allButton[21] = ui->Han_Xiang2;
    allButton[22] = ui->Han_Shi1;
    allButton[23] = ui->Han_Shi2;
    allButton[24] = ui->Han_Jiang;
    allButton[25] = ui->Han_Pao1;
    allButton[26] = ui->Han_Pao2;
    allButton[27] = ui->Han_Bing1;
    allButton[28] = ui->Han_Bing2;
    allButton[29] = ui->Han_Bing3;
    allButton[30] = ui->Han_Bing4;
    allButton[31] = ui->Han_Bing5;
    for (int e = 0; e < 32; e++)
    {
        allButton[e]->installEventFilter(this);
    }
    for (int i = 0; i < 32; i++)
    {
        if (i < 16)
        {
            qiziIsChuOrHan[i] = "Chu";
        }
        else
        {
            qiziIsChuOrHan[i] = "Han";
        }
    }
    connect(&autoTimer, SIGNAL(timeout()), this, SLOT(xiangqitimeEvent()));
    ui->pvp_radioButton->setChecked(true);
    ui->Continue->setEnabled(false);
    connect(&myProcess, SIGNAL(readyReadStandardOutput()), this, SLOT(on_readoutput()));
    connect(ui->actionLocal, SIGNAL(triggered()), this, SLOT(choosePy()));
    connect(ui->actionLocal_bin, SIGNAL(triggered()), this, SLOT(chooseBin()));
    connect(ui->actionAbout, SIGNAL(triggered()), this, SLOT(myabout()));
}

MainWindow::~MainWindow()
{
    myProcess.kill();
    myProcess.waitForFinished();
    delete ui;
}

preStep::preStep(QPushButton *button, QPoint point)
{
    preButton = button;
    prePoint = point;
}

preStep::preStep()
{
    preButton = nullptr;
    prePoint = QPoint(0, 0);
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
    if (event->type() == QEvent::MouseButtonPress)
    {
        if (ifOver != "none")
        {
            QMessageBox plzBox;
            plzBox.setText("请先重置对局。");
            plzBox.exec();
            return true;
        }
        if (Pause)
        {
            QMessageBox Box;
            Box.setText("已暂停");
            Box.exec();
            return true;
        }
        if ((autoNum == -1 && chuhanRound == "Chu") || (autoNum == 1 && chuhanRound == "Han") || autoNum == 2)
        {
            QMessageBox Box;
            Box.setText("请勿干扰电脑下棋。");
            Box.exec();
            return true;
        }
        autoTimer.stop();
        myProcess.kill();
        myProcess.waitForFinished();
        autoSum = 0;
    }
    for (int B = 0; B < 32; B++)
    {
        if (obj == allButton[B])
        {
            if (event->type() == QEvent::MouseButtonPress)
            {
                chButton = allButton[B];
                ifPressed = true;
                chButton->raise();
                if (!CheckRoundCorrectAndSetRound())
                {
                    return false;
                }
                cStep = preStep(chButton, QPoint(chButton->x(), chButton->y()));
                relaPoint = QPoint(mouseEvent->x(), mouseEvent->y());
                return false;
            }
            else if (event->type() == QEvent::MouseMove && ifPressed)
            {
                chButton->move(mouseEvent->x() + chButton->x() - relaPoint.x(), mouseEvent->y() + chButton->y() - relaPoint.y());
                return false;
            }
            break;
        }
    }
    /*
    这一部分原本的内容已经被删了，上面的循环是其代替实现内容。
    */
    if (event->type() == QEvent::MouseButtonRelease)
    {
        autoTimer.start(1000);
        if (chButton == nullptr || cStep.preButton == nullptr)
        {
            return false;
        }
        ifPressed = false;
        int rx, ry;
        rx = (chButton->x() / 68 + 1) / 2;
        ry = (chButton->y() / 39.5 + 1) / 2;
        if (rx > 4)
        {
            rx = 9 - ((1233 - chButton->x()) / 68 + 1) / 2;
        }
        else if (rx < 0)
        {
            rx = 0;
        }
        if (rx > 9)
        {
            rx = 9;
        }
        if (ry < 0)
        {
            ry = 0;
        }
        else if (ry > 8)
        {
            ry = 8;
        }
        int rw, rh;
        for (rh = 0; rh < 9; rh++)
        {
            for (rw = 0; rw < 10; rw++)
            {
                if (cStep.prePoint == qipanCoordinates[rh][rw])
                {
                    goto nextStep1;
                }
            }
        }
    nextStep1:
        int pWidth, pHeight;
        pWidth = rx - rw;
        pHeight = ry - rh;
        if (pWidth == 0 && pHeight == 0)
        {
            cStep.preButton->move(cStep.prePoint);
            cStep = preStep();
            chuhanRound = PrechuhanRound;
            return false;
        }
        if (chButton == allButton[0] || chButton == allButton[1] || chButton == allButton[16] || chButton == allButton[17])
        {

            if (pWidth == 0 || pHeight == 0)
            {
                if (pWidth != 0)
                {
                    for (int i = rx; i - rw != 0; i -= pWidth / absoluteValue(pWidth))
                    {
                        for (int s = 0; s < 32; s++)
                        {
                            if (QPoint(allButton[s]->x(), allButton[s]->y()) == qipanCoordinates[ry][i] && i != rx)
                            {
                                ShowfBoxAndQuash("您的落点是错误的，您不应该让车跨过棋子行进。");
                                return false;
                            }
                        }
                    }
                }
                if (pHeight != 0)
                {
                    for (int i = ry; i - rh != 0; i -= pHeight / absoluteValue(pHeight))
                    {
                        for (int s = 0; s < 32; s++)
                        {
                            if (QPoint(allButton[s]->x(), allButton[s]->y()) == qipanCoordinates[i][rx] && i != ry)
                            {
                                ShowfBoxAndQuash("您的落点是错误的，您不应该让车跨过棋子行进。");
                                return false;
                            }
                        }
                    }
                }
            }
            else
            {
                ShowfBoxAndQuash("您的落点是错误的，车只能直行。");
                return false;
            }
        }
        if (chButton == allButton[2] || chButton == allButton[3] || chButton == allButton[18] || chButton == allButton[19])
        {

            if (!((absoluteValue(pWidth) == 2 && absoluteValue(pHeight) == 1) || (absoluteValue(pWidth) == 1 && absoluteValue(pHeight) == 2)))
            {
                ShowfBoxAndQuash("您的落点是错误的，马只能走日。");
                return false;
            }
            else if (absoluteValue(pWidth) == 2)
            {
                for (int i = 0; i < 32; i++)
                {
                    if (QPoint(allButton[i]->x(), allButton[i]->y()) == qipanCoordinates[rh][rw + pWidth / absoluteValue(pWidth)])
                    {
                        ShowfBoxAndQuash("您的落点是错误的，拌马脚了。");
                        return false;
                    }
                }
            }
            else if (absoluteValue(pHeight) == 2)
            {
                for (int i = 0; i < 32; i++)
                {
                    if (QPoint(allButton[i]->x(), allButton[i]->y()) == qipanCoordinates[rh + pHeight / absoluteValue(pHeight)][rw])
                    {
                        ShowfBoxAndQuash("您的落点是错误的，拌马脚了。");
                        return false;
                    }
                }
            }
        }
        if (chButton == allButton[4] || chButton == allButton[5] || chButton == allButton[20] || chButton == allButton[21])
        {
            if (chButton == allButton[4] || chButton == allButton[5])
            {
                if (rx > 4)
                {
                    ShowfBoxAndQuash("您的落点是错误的，相不能越界。");
                    return false;
                }
            }
            else if (chButton == allButton[20] || chButton == allButton[21])
            {
                if (rx <= 4)
                {
                    ShowfBoxAndQuash("您的落点是错误的，相不能越界。");
                    return false;
                }
            }

            if (!(absoluteValue(pWidth) == 2 && absoluteValue(pHeight) == 2))
            {
                ShowfBoxAndQuash("您的落点是错误的，相只能走田。");
                return false;
            }
            for (int i = 0; i < 32; i++)
            {
                if (QPoint(allButton[i]->x(), allButton[i]->y()) == qipanCoordinates[rh + pHeight / absoluteValue(pHeight)][rw + pWidth / absoluteValue(pWidth)])
                {
                    ShowfBoxAndQuash("您的落点是错误的，塞相眼了。");
                    return false;
                }
            }
        }
        if (chButton == allButton[6] || chButton == allButton[7] || chButton == allButton[22] || chButton == allButton[23])
        {
            if (chButton == allButton[6] || chButton == allButton[7])
            {
                if (rx > 2 || ry < 3 || ry > 5)
                {
                    ShowfBoxAndQuash("您的落点是错误的，士不能出格。");
                    return false;
                }
            }
            else if (chButton == allButton[22] || chButton == allButton[23])
            {
                if (rx < 7 || ry < 3 || ry > 5)
                {
                    ShowfBoxAndQuash("您的落点是错误的，士不能出格。");
                    return false;
                }
            }
            if (absoluteValue(pWidth) != 1 || absoluteValue(pHeight) != 1)
            {
                ShowfBoxAndQuash("您的落点是错误的，士只能走斜线。");
                return false;
            }
        }
        if (chButton == allButton[8] || chButton == allButton[24])
        {
            if (chButton == allButton[8])
            {
                if (rx > 2 || ry < 3 || ry > 5)
                {
                    ShowfBoxAndQuash("您的落点是错误的，将不能出格。");
                    return false;
                }
            }
            if (chButton == allButton[24])
            {
                if (rx < 7 || ry < 3 || ry > 5)
                {
                    ShowfBoxAndQuash("您的落点是错误的，将不能出格。");
                    return false;
                }
            }
            if (!(absoluteValue(pWidth) == 0 && absoluteValue(pHeight) == 1) && !(absoluteValue(pWidth) == 1 && absoluteValue(pHeight) == 0))
            {
                ShowfBoxAndQuash("您的落点是错误的，将只能直走一格。");
                return false;
            }
        }
        if (chButton == allButton[9] || chButton == allButton[10] || chButton == allButton[25] || chButton == allButton[26])
        {
            if (pWidth == 0 || pHeight == 0)
            {
                int ifexq = 0;
                bool iffinexq = false;
                if (pWidth != 0)
                {

                    for (int i = rx; i - rw != 0; i -= pWidth / absoluteValue(pWidth))
                    {
                        for (int s = 0; s < 32; s++)
                        {
                            if (QPoint(allButton[s]->x(), allButton[s]->y()) == qipanCoordinates[ry][i] && i != rx)
                            {
                                ifexq += 1;
                            }
                            if (QPoint(allButton[s]->x(), allButton[s]->y()) == qipanCoordinates[ry][i] && i == rx)
                            {
                                iffinexq = true;
                            }
                        }
                    }
                    if (ifexq > 1)
                    {
                        ShowfBoxAndQuash("您的落点是错误的，炮最多只能翻越一个棋子。");
                        return false;
                    }
                    if (ifexq == 1 && !iffinexq)
                    {
                        ShowfBoxAndQuash("您的落点是错误的，炮不能只翻过棋子而不吃棋子。");
                        return false;
                    }
                    if (!ifexq && iffinexq)
                    {
                        ShowfBoxAndQuash("您的落点是错误的，炮不能不翻过棋子而吃掉棋子。");
                        return false;
                    }
                }
                if (pHeight != 0)
                {
                    for (int i = ry; i - rh != 0; i -= pHeight / absoluteValue(pHeight))
                    {
                        for (int s = 0; s < 32; s++)
                        {
                            if (QPoint(allButton[s]->x(), allButton[s]->y()) == qipanCoordinates[i][rx] && i != ry)
                            {

                                ifexq += 1;
                            }
                            if (QPoint(allButton[s]->x(), allButton[s]->y()) == qipanCoordinates[i][rx] && i == ry)
                            {
                                iffinexq = true;
                            }
                        }
                    }
                    if (ifexq > 1)
                    {
                        ShowfBoxAndQuash("您的落点是错误的，炮最多只能翻越一个棋子。");
                        return false;
                    }
                    if (ifexq == 1 && !iffinexq)
                    {
                        ShowfBoxAndQuash("您的落点是错误的，炮不能只翻过棋子而不吃棋子。");
                        return false;
                    }
                    if (!ifexq && iffinexq)
                    {
                        ShowfBoxAndQuash("您的落点是错误的，炮不能不翻过棋子而吃掉棋子。");
                        return false;
                    }
                }
            }
            else
            {
                ShowfBoxAndQuash("您的落点是错误的，炮只能直行。");
                return false;
            }
        }
        if (chButton == allButton[11] || chButton == allButton[12] || chButton == allButton[13] || chButton == allButton[14] || chButton == allButton[15] || chButton == allButton[27] || chButton == allButton[28] || chButton == allButton[29] || chButton == allButton[30] || chButton == allButton[31])
        {
            if (chButton == allButton[11] || chButton == allButton[12] || chButton == allButton[13] || chButton == allButton[14] || chButton == allButton[15])
            {
                if (rx <= 4 && absoluteValue(pHeight))
                {
                    ShowfBoxAndQuash("您的落点是错误的，兵在过河前不能左右行动。");
                    return false;
                }
                if (pWidth < 0)
                {
                    ShowfBoxAndQuash("您的落点是错误的，兵在任何情况下不能后退。");
                    return false;
                }
            }
            if (chButton == allButton[27] || chButton == allButton[28] || chButton == allButton[29] || chButton == allButton[30] || chButton == allButton[31])
            {
                if (rx > 4 && absoluteValue(pHeight))
                {
                    ShowfBoxAndQuash("您的落点是错误的，兵在过河前不能左右行动。");
                    return false;
                }
                if (pWidth > 0)
                {
                    ShowfBoxAndQuash("您的落点是错误的，兵在任何情况下不能后退。");
                    return false;
                }
            }
            if (!(absoluteValue(pWidth) == 0 && absoluteValue(pHeight) == 1) && !(absoluteValue(pWidth) == 1 && absoluteValue(pHeight) == 0))
            {
                ShowfBoxAndQuash("您的落点是错误的，兵只能直走一格。");
                return false;
            }
        }
        // 以下内容可能有问题，有一次不知怎么的一个黑方小兵竟然吃了另一个黑方小兵，但是奇怪的是此后又恢复正常了，我也无法复现...
        bool ifexate = false;
        preStep preyiqvshi;
        if (yiqvshiStep.preButton != nullptr)
        {
            preyiqvshi = yiqvshiStep;
        }
        for (int i = 0; i < 32; i++)
        {
            if (QPoint(allButton[i]->x(), allButton[i]->y()) == qipanCoordinates[ry][rx] && allButton[i] != chButton)
            {
                ifexate = true;
                for (int s = 0; s < 32; s++)
                {
                    if (allButton[s] == chButton && qiziIsChuOrHan[s] == qiziIsChuOrHan[i])
                    {
                        ShowfBoxAndQuash("您的落点是错误的，您不应该自己吃自己的棋子。");
                        return false;
                    }
                    else if (allButton[s] == chButton && qiziIsChuOrHan[s] != qiziIsChuOrHan[i])
                    {
                        break;
                    }
                }
                yiqvshiStep = preStep(allButton[i], QPoint(allButton[i]->x(), allButton[i]->y()));
                allButton[i]->move(-114, -514);
                if (allButton[i] == ui->Chu_Jiang)
                {
                    ifOver = "Chu";
                }
                else if (allButton[i] == ui->Han_Jiang)
                {
                    ifOver = "Han";
                }
                break;
            }
        }
        chButton->move(qipanCoordinates[ry][rx]);
        if (!ifexate)
        {
            yiqvshiStep = preStep();
        }
        if (ui->Chu_Jiang->y() == ui->Han_Jiang->y())
        {
            bool ifexstj = false;
            for (int i = 0; i < 32; i++)
            {
                if (allButton[i]->y() == ui->Chu_Jiang->y() && allButton[i]->x() > ui->Chu_Jiang->x() && allButton[i]->x() < ui->Han_Jiang->x() && allButton[i] != ui->Chu_Jiang && allButton[i] != ui->Han_Jiang)
                {
                    ifexstj = true;
                    break;
                }
            }
            if (!ifexstj)
            {
                ShowfBoxAndQuash("禁止移动！两将不能见面！");
                if (yiqvshiStep.preButton != nullptr)
                {
                    yiqvshiStep.preButton->move(yiqvshiStep.prePoint);
                    yiqvshiStep = preyiqvshi;
                }
                return false;
            }
        }
        if (ifOver == "none")
        {
            huiqiStep = cStep;
        }
        else
        {
            huiqiStep = preStep();
            yiqvshiStep = preStep();
        }
        if (ifOver == "Chu")
        {
            QMessageBox::StandardButton result = QMessageBox::information(NULL, "胜负已分！", "恭喜红方获胜！\n是否要重置对局？", QMessageBox::Yes | QMessageBox::No);
            switch (result)
            {
            case QMessageBox::Yes:
                for (int i = 0; i < 32; i++)
                {
                    allButton[i]->move(qiziCoordinate[i]);
                }
                ifOver = "none";
                chuhanRound = "none";
                PrechuhanRound = "none";
                MainWindow::on_Continue_clicked();
                break;
            default:
                break;
            }
        }
        else if (ifOver == "Han")
        {
            QMessageBox::StandardButton result = QMessageBox::information(NULL, "胜负已分！", "恭喜黑方获胜！\n是否要重置对局？", QMessageBox::Yes | QMessageBox::No);
            switch (result)
            {
            case QMessageBox::Yes:
                for (int i = 0; i < 32; i++)
                {
                    allButton[i]->move(qiziCoordinate[i]);
                }
                ifOver = "none";
                chuhanRound = "none";
                PrechuhanRound = "none";
                MainWindow::on_Continue_clicked();
                break;
            default:
                break;
            }
        }
        chButton = nullptr;
        return false;
    }
    return false;
}

void MainWindow::on_Start_clicked()
{
    QMessageBox::StandardButton result = QMessageBox::information(NULL, "开始", "是否要重置对局？", QMessageBox::Yes | QMessageBox::No);
    switch (result)
    {
    case QMessageBox::Yes:
        myProcess.kill();
        myProcess.waitForFinished();
        myProcess.readAll();
        ifReadyRead = false;
        autoSum = 0;
        for (int i = 0; i < 32; i++)
        {
            allButton[i]->move(qiziCoordinate[i]);
        }
        ifOver = "none";
        chuhanRound = "none";
        PrechuhanRound = "none";
        yiqvshiStep = preStep();
        huiqiStep = preStep();
        MainWindow::on_Continue_clicked();
        break;
    default:
        break;
    }
}

void MainWindow::on_Pause_clicked()
{
    if (ifOver != "none")
    {
        QMessageBox plzBox;
        plzBox.setText("对局都结束了，点暂停有用吗？");
        plzBox.exec();
        return;
    }
    ui->Pause->setEnabled(false);
    Pause = true;
    ui->label_3->setText("已暂停");
    autoTimer.stop();
    myProcess.kill();
    myProcess.waitForFinished();
    myProcess.readAll();
    autoSum = 0;
    ifReadyRead = false;
    ui->Continue->setEnabled(true);
}

void MainWindow::on_Continue_clicked()
{
    Pause = false;
    ui->label_3->setText("已取消暂停");
    if (!ui->pvp_radioButton->isChecked())
    {
        autoTimer.start(1000);
    }
    ui->Pause->setEnabled(true);
    ui->Continue->setEnabled(false);
}

void MainWindow::on_Repent_clicked()
{
    if (ifOver != "none")
    {
        QMessageBox plzBox;
        plzBox.setText("请先重置对局。");
        plzBox.exec();
        return;
    }
    if (huiqiStep.preButton == nullptr)
    {
        QMessageBox huiqiBox;
        huiqiBox.setText("您无棋可悔");
        huiqiBox.exec();
        return;
    }
    MainWindow::setEnabled(false);
    autoTimer.stop();
    myProcess.kill();
    myProcess.waitForFinished();
    myProcess.readAll();
    autoSum = 0;
    ifReadyRead = false;
    huiqiStep.preButton->move(huiqiStep.prePoint);
    if (yiqvshiStep.preButton != nullptr)
    {
        yiqvshiStep.preButton->move(yiqvshiStep.prePoint);
        yiqvshiStep = preStep();
    }
    huiqiStep = preStep();
    if (chuhanRound == "Chu")
    {
        chuhanRound = "Han";
    }
    else
    {
        chuhanRound = "Chu";
    }
    autoTimer.start(1000);
    MainWindow::setEnabled(true);
}

void MainWindow::on_pve_radioButton_clicked()
{
    if (ifOver != "none")
    {
        QMessageBox plzBox;
        plzBox.setText("请先重置对局。");
        plzBox.exec();
        return;
    }
    // bool ifTimerIsActive = autoTimer.isActive();
    QStringList items = {"楚（黑方）", "汉（红方）"};
    QString item = QInputDialog::getItem(this, "人机对战", "请选择作为电脑的一方:", items, 0, false);
    if (item == "楚（黑方）")
    {
        autoNum = -1;
    }
    else
    {
        autoNum = 1;
    }
    ui->label_3->setText("正在执行切换，请耐心等待...");
    MainWindow::setEnabled(false);
    myProcess.kill();
    myProcess.waitForFinished();
    myProcess.readAll();
    autoSum = 0;
    ifReadyRead = false;
    MainWindow::setEnabled(true);
    ui->label_3->setText("切换完毕！");
    if (!Pause)
    {
        autoTimer.start(1000);
    }
}

void MainWindow::on_eve_radioButton_clicked()
{
    if (ifOver != "none")
    {
        QMessageBox plzBox;
        plzBox.setText("请先重置对局。");
        plzBox.exec();
        return;
    }
    autoNum = 2;
    ui->label_3->setText("正在执行切换，请耐心等待...");
    MainWindow::setEnabled(false);
    myProcess.kill();
    myProcess.waitForFinished();
    myProcess.readAll();
    autoSum = 0;
    ifReadyRead = false;
    MainWindow::setEnabled(true);
    ui->label_3->setText("切换完毕！");
    if (!Pause)
    {
        autoTimer.start(1000);
    }
}

void MainWindow::on_comboBox_currentIndexChanged(int index)
{
    comboIndex = index;
}

void MainWindow::on_Save_clicked()
{
    if (ifOver != "none")
    {
        QMessageBox plzBox;
        plzBox.setText("请先重置对局。");
        plzBox.exec();
        return;
    }
    autoTimer.stop();
    myProcess.kill();
    myProcess.waitForFinished();
    myProcess.readAll();
    ifReadyRead = false;
    autoSum = 0;
    if (!fs::exists(saveDir))
    {
        fs::create_directory(saveDir);
    }
    else
    {
        qDebug() << "Existed!Pass!";
    }
    bool ifOK = false;
    QString savedFileName = QInputDialog::getText(NULL, "保存", "请输入存档名(不要带有空格!):", QLineEdit::Normal, "", &ifOK);
    if (!ifOK)
    {
        QMessageBox msgBox;
        msgBox.setText("ERROR!");
        msgBox.exec();
        return;
    }
    savedFileName += ".can";
    std::ofstream fout;
    fout.open("./Saved/" + savedFileName.toLocal8Bit().toStdString(), std::ios::out);
    if (!fout.is_open())
    {
        QMessageBox msgBox;
        msgBox.setText("ERROR!");
        msgBox.exec();
        fout.close();
        return;
    }
    for (int i = 0; i < 32; i++)
    {
        fout << "(" << allButton[i]->x() << "," << allButton[i]->y() << ")" << std::endl;
    }
    fout << chuhanRound.toStdString();
    fout.close();
    if (!Pause)
    {
        autoTimer.start(1000);
    }
    return;
}

void MainWindow::on_Load_clicked()
{
    autoTimer.stop();
    myProcess.kill();
    myProcess.waitForFinished();
    myProcess.readAll();
    ifReadyRead = false;
    autoSum = 0;
    QString fileLoadName;
    std::string str;
    fileLoadName = QFileDialog::getOpenFileName(this, "选择存档文件", "./", "存档文件(*.can)");
    if (fileLoadName.isEmpty())
    {
        QMessageBox Box;
        Box.setText("没有选中文件!");
        Box.exec();
        return;
    }
    std::ifstream fin;
    fin.open(fileLoadName.toLocal8Bit().toStdString(), std::ios::in);
    if (!fin.is_open())
    {
        QMessageBox msgBox;
        msgBox.setText("ERROR!");
        msgBox.exec();
        fin.close();
        return;
    }
    int a = 0;
    while (getline(fin, str))
    {
        if (a < 32)
        {
            QString toIntTemp;
            int gx, gy, s;
            for (int i = 1; str[i] != ','; i++)
            {
                s = i;
                toIntTemp += str[i];
                qDebug() << QString::fromStdString(str) << "里" << QString(str[i]) << "进入流";
                if (i > 10) // 如遇到文件错误也能够退出循环
                {
                    QMessageBox msgBox;
                    msgBox.setText("ERROR!");
                    msgBox.exec();
                    return;
                }
            }
            gx = toIntTemp.toInt();
            toIntTemp = QString();
            for (int i = s + 2; str[i] != ')'; i++)
            {
                toIntTemp += str[i];
                qDebug() << QString::fromStdString(str) << "里" << QString(str[i]) << "进入流";
                if (i > 14) // 如遇到文件错误也能够退出循环
                {
                    QMessageBox msgBox;
                    msgBox.setText("ERROR!");
                    msgBox.exec();
                    return;
                }
            }
            gy = toIntTemp.toInt();
            toIntTemp = QString();
            allButton[a]->move(gx, gy);
        }
        else if (a == 32)
        {
            // 考虑到文件内容可能会出问题，故采用此写法。
            if (str == "Chu")
            {
                chuhanRound = "Chu";
            }
            else if (str == "Han")
            {
                chuhanRound = "Han";
            }
            else
            {
                chuhanRound = "none";
            }
        }
        a++;
    }
    fin.close();
    ifOver = "none";
    PrechuhanRound = "none";
    yiqvshiStep = preStep();
    huiqiStep = preStep();
    MainWindow::on_Continue_clicked();
    if (!Pause)
    {
        autoTimer.start(1000);
    }
    return;
}

void MainWindow::on_pvp_radioButton_clicked()
{
    autoSum = 0;
    autoNum = 0;
    autoTimer.stop();
    ui->label_3->setText("正在执行切换，请耐心等待...");
    MainWindow::setEnabled(false);
    myProcess.kill();
    myProcess.waitForFinished();
    myProcess.readAll();
    ifReadyRead = false;
    MainWindow::setEnabled(true);
    ui->label_3->setText("切换完毕！");
}

void MainWindow::chooseBin()
{
    qDebug() << "RUN chooseBin()";
    on_Pause_clicked();
    QString tempPath;
    tempPath = QFileDialog::getOpenFileName(this, "选择二进制文件", "./", "二进制文件(*.que)");
    if (tempPath.isEmpty())
    {
        ui->actionLocal->setChecked(true);
        ui->actionLocal_bin->setChecked(false);
        NetBinPath = QString();
        QMessageBox Box;
        Box.setText("没有选中文件!");
        Box.exec();
        return;
    }
    NetBinPath = tempPath;
    ui->actionLocal->setChecked(false);
}

void MainWindow::choosePy()
{
    qDebug() << "RUN choosePy()";
    NetBinPath = QString();
    on_Pause_clicked();
    ui->actionLocal_bin->setChecked(false);
}

void MainWindow::myabout()
{
    QMessageBox aboBox;
    aboBox.setText("Made by Chen-197\nLicense: GPL-3.0");
    aboBox.exec();
}
