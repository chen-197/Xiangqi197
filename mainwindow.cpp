#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QPointer>
#include <QUrlQuery>
#include <QSettings>
#include <QDialog>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QRegularExpression>
#include <QSpinBox>
#include <QCheckBox>
#include <cctype>
#include <string>
#include <algorithm>
#include <limits>

int clickedButtonNum = -1;
QPoint mousePoint;
QPoint relaPoint;
QPoint beginButtonPoint;
QPoint pbPoint;
QPoint qipanCoordinates[9][10] = {};
QPoint qiziCoordinate[32] = {};
QString qiziIsChuOrHan[32];
QString chuhanRound = "none";
QString PrechuhanRound = "none";
QString ifOver = "none";
QString FEN;
QString NetBinPath;
fs::path saveDir = "Saved";
QPushButton *allButton[32];
QPushButton *chButton = nullptr;
QTimer* autoTimer = nullptr;
QPointer<QNetworkReply> reply;
QByteArray readInfo;
QNetworkAccessManager* tempManager = nullptr;
QNetworkRequest request;
AllST Steps;
AllST StepsBak;
//QProcess myProcess;
preStep yiqvshiStep;
preStep huiqiStep;
preStep cStep;
int autoNum = 0;
int autoSum = 0;
int SumBak = 0;
int saveNum = 0;
int distance[4];
int difficulty = 9;
int comboIndex = 0;
bool ifPressed = false;
bool Pause = false;
bool ifReadyRead = false;
bool repl = false;
double ti;
char abc[] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i'};


// ==== UI scaling / layout config ====
// The original UI was designed at 1299x796 in mainwindow.ui (absolute geometries).
// Edit these two numbers to change the default window size at startup.
static int g_uiStartWidth  = 1299;
static int g_uiStartHeight = 796;

// Keep aspect ratio when scaling the whole UI (recommended: true).
static bool g_uiKeepAspect = true;

// Base board geometry in the original design (pixels, relative to ui->centralwidget).
static constexpr int kBaseStepX = 136;
static constexpr int kBaseStepY = 79;
static constexpr int kBaseRiverGap = 145; // extra gap between ranks 4 and 5
static constexpr int kBasePieceSize = 68;

static QPoint capturedOffboardPoint()
{
    // Always far off-screen regardless of scaling.
    return QPoint(-10000, -10000);
}

static const QPoint kBaseQiziCoordinate[32] = {
    QPoint(0, 0), QPoint(0, 632), QPoint(0, 79), QPoint(0, 553), QPoint(0, 158), QPoint(0, 474), QPoint(0, 237), QPoint(0, 395),
    QPoint(0, 316), QPoint(272, 79), QPoint(272, 553), QPoint(408, 0), QPoint(408, 158), QPoint(408, 316), QPoint(408, 474), QPoint(408, 632),
    QPoint(1233, 0), QPoint(1233, 632), QPoint(1233, 79), QPoint(1233, 553), QPoint(1233, 158), QPoint(1233, 474), QPoint(1233, 237), QPoint(1233, 395),
    QPoint(1233, 316), QPoint(961, 79), QPoint(961, 553), QPoint(825, 0), QPoint(825, 158), QPoint(825, 316), QPoint(825, 474), QPoint(825, 632)
};

// Initial piece cell indices (file 0..8, rank 0..9). Computed from kBaseQiziCoordinate vs base grid.
static int g_initFile[32];
static int g_initRank[32];
static bool g_initCellsReady = false;

static QPoint baseGridPoint(int file, int rank)
{
    // file: 0..8 maps to Y, rank: 0..9 maps to X (project's coordinate convention)
    const int y = file * kBaseStepY;
    int x = 0;
    if (rank <= 4)
        x = rank * kBaseStepX;
    else
        x = 4 * kBaseStepX + kBaseRiverGap + (rank - 5) * kBaseStepX;
    return QPoint(x, y);
}

static void ensureInitCells()
{
    if (g_initCellsReady) return;
    for (int i = 0; i < 32; ++i) { g_initFile[i] = -1; g_initRank[i] = -1; }
    for (int i = 0; i < 32; ++i)
    {
        const QPoint p = kBaseQiziCoordinate[i];
        for (int file = 0; file < 9; ++file)
        {
            for (int rank = 0; rank < 10; ++rank)
            {
                if (p == baseGridPoint(file, rank))
                {
                    g_initFile[i] = file;
                    g_initRank[i] = rank;
                    file = 9; // break outer
                    break;
                }
            }
        }
    }
    g_initCellsReady = true;
}


// --- Cloudbook rule (queryrule) helpers ---
// API doc: action=queryrule requires a starting FEN (board) and a movelist (>= 4 moves).
// We keep a "rule base" snapshot (positions + side-to-move) and can send only the last few moves
// by reconstructing the start position for that tail segment.
static QVector<QPoint> g_ruleBasePositions;
static QString g_ruleBaseFen;
static bool g_ruleBaseReady = false;
static inline QPoint kCapturedOffboard = capturedOffboardPoint();
static const QPoint kCapturedCell(-1, -1); // must match analysisStep() capture hiding
static int g_ruleTailMoves = 12;              // queryrule tail moves (>=4)
static int g_ruleRepTimes = 1;                // queryrule reptimes (1~10)
static bool g_ruleAvoidDraw = false;          // if true, treat rule:draw as ban
static bool g_ruleEnable = true;              // if false, skip queryrule entirely

// Forward declarations
QString getFEN();
int charToqipanInt(char charK);
void resetRuleBase();
static QString fenFromPositions(const QVector<QPoint>& pos, QChar sideToMove);
static bool applyOneRecordedStep(QVector<QPoint>& pos, const QString& step);
static bool buildQueryruleInputs(int tailMoves, QString& outStartFen, QString& outMovelist);
static QString banFromQueryruleResponse(const QByteArray& ruleResp);

void savStep(int ty, int tx, AllST &Steps){
    QStringList tmp;
    if(!repl) tmp = Steps.s; else tmp = Steps.s_tmp;
    int s = tmp.length();
    tmp.append("move:");
    for(int i = 0; i < 9; i++)
    {
        for(int a = 0; a < 10; a++)
        {
            if(cStep.prePoint == qipanCoordinates[i][a]) tmp[s]+=(abc[i]+QString::number(9-a));
        }
    }
    tmp[s]+=(abc[ty]+QString::number(9-tx));
    if(!yiqvshiStep.preButton) tmp[s]+="NU"; else{
        for(int k = 0; k <= 31; k++)
        {
            if(yiqvshiStep.preButton == allButton[k])
            {
                if(k < 10) tmp[s]+="0";
                tmp[s]+=QString::number(k);
                break;
            }
        }
    } if(!repl) Steps.s = tmp; else Steps.s_tmp = tmp;
    //qDebug() << tmp[s][6].toLatin1();
}

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
    autoTimer->stop();
    ifPressed = false;
    chuhanRound = PrechuhanRound;
    if (cStep.preButton == nullptr)
    {
        //qDebug() << "ERROR!";
        return;
    }
    QMessageBox fBox;
    fBox.setText(text);
    fBox.exec();
    // Snap back to the nearest valid cell. This is more robust than restoring
    // the raw pixel coordinate when UI scaling/rounding is involved.
    if (cStep.prePoint.x() < -1000 || cStep.prePoint.y() < -1000)
    {
        cStep.preButton->move(capturedOffboardPoint());
    }
    else
    {
        qint64 best = std::numeric_limits<qint64>::max();
        QPoint bestPt = qipanCoordinates[0][0];
        for (int f = 0; f < 9; ++f)
        {
            for (int r = 0; r < 10; ++r)
            {
                const QPoint gp = qipanCoordinates[f][r];
                const qint64 dx = cStep.prePoint.x() - gp.x();
                const qint64 dy = cStep.prePoint.y() - gp.y();
                const qint64 d = dx * dx + dy * dy;
                if (d < best)
                {
                    best = d;
                    bestPt = gp;
                }
            }
        }
        cStep.preButton->move(bestPt);
    }
    cStep = preStep();
    autoTimer->start(1000);
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
    //qDebug() << FEN_TEMP;
    return FEN_TEMP;
}
int charToqipanInt(char charK)
{
    for (int i = 0; i < sizeof(abc) / sizeof(abc[0]); i++)
    {
        if (abc[i] == charK)
        {
            return i;
        }
    }
    return -1;
}


void resetRuleBase()
{
    // Store rule-base positions in *logical cells* (file 0..8, rank 0..9), independent of UI scaling.
    // Captured pieces are stored as (-1,-1).
    g_ruleBasePositions.resize(32);

    auto pixelToCell = [](const QPoint& p) -> QPoint
    {
        if (p.x() < -1000 || p.y() < -1000) return kCapturedCell;

        // Exact match first.
        for (int file = 0; file < 9; ++file)
        {
            for (int rank = 0; rank < 10; ++rank)
            {
                if (p == qipanCoordinates[file][rank])
                {
                    return QPoint(file, rank);
                }
            }
        }

        // Fallback: nearest cell.
        qint64 best = std::numeric_limits<qint64>::max();
        int bf = 0, br = 0;
        for (int file = 0; file < 9; ++file)
        {
            for (int rank = 0; rank < 10; ++rank)
            {
                const QPoint gp = qipanCoordinates[file][rank];
                const qint64 dx = p.x() - gp.x();
                const qint64 dy = p.y() - gp.y();
                const qint64 d = dx * dx + dy * dy;
                if (d < best)
                {
                    best = d;
                    bf = file;
                    br = rank;
                }
            }
        }
        return QPoint(bf, br);
    };

    for (int i = 0; i < 32; ++i)
    {
        if (allButton[i])
        {
            g_ruleBasePositions[i] = pixelToCell(QPoint(allButton[i]->x(), allButton[i]->y()));
        }
        else
        {
            g_ruleBasePositions[i] = kCapturedCell;
        }
    }

    g_ruleBaseFen = getFEN();
    g_ruleBaseReady = true;
}

static QChar sideFromFen(const QString& fen)
{
    const int sp = fen.lastIndexOf(' ');
    if (sp >= 0 && sp + 1 < fen.size())
    {
        const QChar c = fen[sp + 1];
        if (c == QChar('w') || c == QChar('b')) return c;
    }
    return QChar('w');
}

static QChar toggleSide(QChar c)
{
    return (c == QChar('w')) ? QChar('b') : QChar('w');
}


static QString fenFromPositions(const QVector<QPoint>& pos, QChar sideToMove)
{
    // pos[i] is a logical cell QPoint(file, rank), or kCapturedCell for captured.
    QString fen;
    for (int rank = 0; rank < 10; ++rank)
    {
        int spacenum = 0;
        for (int file = 0; file < 9; ++file)
        {
            bool hasPiece = false;
            int pieceIdx = -1;
            const QPoint cell(file, rank);

            for (int k = 0; k < 32; ++k)
            {
                if (pos[k] == cell)
                {
                    hasPiece = true;
                    pieceIdx = k;
                    break;
                }
            }

            if (hasPiece)
            {
                if (spacenum != 0)
                {
                    fen += QString::number(spacenum);
                    spacenum = 0;
                }
                fen += QChar(qiziCharac(allButton[pieceIdx]));
            }
            else
            {
                spacenum++;
            }

            if (file == 8 && spacenum != 0)
            {
                fen += QString::number(spacenum);
            }
        }
        if (rank != 9) fen += "/";
    }
    fen += " ";
    fen += sideToMove;
    return fen;
}


static bool coordToCell(QChar fileChar, QChar digitChar, QPoint& outCell)
{
    const int file = charToqipanInt(fileChar.toLatin1());
    const int digit = digitChar.digitValue();
    if (file < 0 || file >= 9 || digit < 0 || digit > 9) return false;
    const int rank = 9 - digit;
    if (rank < 0 || rank >= 10) return false;
    outCell = QPoint(file, rank);
    return true;
}



static bool applyOneRecordedStep(QVector<QPoint>& pos, const QString& step)
{
    if (step.size() < 9 || !step.startsWith("move:")) return false;

    QPoint fromCell, toCell;
    if (!coordToCell(step[5], step[6], fromCell)) return false;
    if (!coordToCell(step[7], step[8], toCell)) return false;

    int mover = -1;
    for (int i = 0; i < 32; ++i)
    {
        if (pos[i] == fromCell)
        {
            mover = i;
            break;
        }
    }
    if (mover == -1) return false;

    // Captured piece index is recorded at [9..10] as "NU" or two digits.
    if (step.size() >= 11 && step.mid(9, 2) != "NU")
    {
        bool ok = false;
        const int capIdx = step.mid(9, 2).toInt(&ok);
        if (ok && capIdx >= 0 && capIdx < 32)
        {
            pos[capIdx] = kCapturedCell;
        }
    }

    pos[mover] = toCell;
    return true;
}

static bool buildQueryruleInputs(int tailMoves, QString& outStartFen, QString& outMovelist)
{
    if (!g_ruleBaseReady || g_ruleBasePositions.size() != 32) return false;

    const QStringList stepList = repl ? Steps.s_tmp : Steps.s;
    const int total = stepList.size();
    if (total < 4) return false; // API requires >= 4 moves 

    int take = tailMoves;
    if (take <= 0 || take > total) take = total;
    if (take < 4) take = 4;
    const int startIndex = total - take;

    QVector<QPoint> pos = g_ruleBasePositions;

    // Reconstruct the start position for the tail segment
    for (int i = 0; i < startIndex; ++i)
    {
        if (!applyOneRecordedStep(pos, stepList[i]))
        {
            return false;
        }
    }

    // Side-to-move at startIndex = base side toggled by number of moves applied.
    QChar side = sideFromFen(g_ruleBaseFen);
    if (startIndex % 2 == 1) side = toggleSide(side);

    outStartFen = fenFromPositions(pos, side);

    QStringList mv;
    mv.reserve(take);
    for (int i = startIndex; i < total; ++i)
    {
        const QString& s = stepList[i];
        if (s.size() < 9 || !s.startsWith("move:")) return false;
        mv.append(s.mid(5, 4)); // "c3c4"
    }
    outMovelist = mv.join("|");
    return true;
}

static QString banFromQueryruleResponse(const QByteArray& ruleResp)
{
    const QString resp = QString::fromUtf8(ruleResp).trimmed();
    if (resp.isEmpty()) return QString();

    // Error cases like "invalid board" / "invalid movelist" / "checkmate" / "stalemate" etc. 
    if (resp.startsWith("invalid") || resp.startsWith("checkmate") || resp.startsWith("stalemate"))
    {
        return QString();
    }

    // Format: move:[MOVE],rule:[RESULT] separated by '|'. RESULT: none/draw/ban 
    QStringList banned;
    const QStringList items = resp.split('|', Qt::SkipEmptyParts);
    for (const QString& it : items)
    {
        const int mi = it.indexOf("move:");
        const int ri = it.indexOf("rule:");
        if (mi < 0 || ri < 0) continue;

        const QString mv = it.mid(mi + 5, 4);
        const QString rl = it.mid(ri + 5).trimmed();

        if (mv.size() == 4 && rl.startsWith("ban"))
        {
            banned.append("move:" + mv);
        }
        else if (g_ruleAvoidDraw && mv.size() == 4 && rl.startsWith("draw"))
        {
            banned.append("move:" + mv);
        }
    }
    return banned.join("|");
}

void MainWindow::analysisStep(const std::string& stepStr, bool ifnRepeat)
{
    // Expected format: "move:<a-i><0-9><a-i><0-9>..."
    auto badStep = [this]() {
        ui->pvp_radioButton->setChecked(true);
        MainWindow::on_pvp_radioButton_clicked();
        autoNum = 0;
        QMessageBox box;
        box.setText("出错了！无法解析步法字符串（存档/云库返回异常）。");
        box.exec();
    };

    if (stepStr.size() < 9 || stepStr.rfind("move:", 0) != 0) {
        badStep();
        return;
    }

    const int charOne = charToqipanInt(stepStr[5]);
    const int charTwo = charToqipanInt(stepStr[7]);

    auto digitToInt = [](char c) -> int {
        return (c >= '0' && c <= '9') ? (c - '0') : -1;
    };
    const int fromDigit = digitToInt(stepStr[6]);
    const int toDigit   = digitToInt(stepStr[8]);

    if (charOne < 0 || charTwo < 0 || fromDigit < 0 || toDigit < 0) {
        badStep();
        return;
    }

    const int intOne = 9 - fromDigit;
    const int intTwo = 9 - toDigit;
    if (intOne < 0 || intOne > 9 || intTwo < 0 || intTwo > 9) {
        badStep();
        return;
    }

    int kq = -1;
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
        ui->pvp_radioButton->setChecked(true);
        MainWindow::on_pvp_radioButton_clicked();
        autoNum = 0;
        QMessageBox box;
        box.setText("出错了！无法定位到棋子！可能是中国象棋云库抽风了！");
        box.exec();
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
            allButton[i]->move(capturedOffboardPoint());
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
        QMessageBox::StandardButton result = QMessageBox::information(this, "胜负已分！",
                                                                     "恭喜红方获胜！\n是否要重置对局？",
                                                                     QMessageBox::Yes | QMessageBox::No);
        switch (result)
        {
        case QMessageBox::Yes:
            for (int i = 0; i < 32; i++)
            {
                allButton[i]->move(qiziCoordinate[i]);
            }
            ifOver = "none";
            chuhanRound = "none";
            Steps.s.clear();
            resetRuleBase();
                PrechuhanRound = "none";
            MainWindow::on_Continue_clicked();
            if(autoNum == 3) on_pvp_radioButton_clicked();
            break;
        case QMessageBox::No:
            ui->Replay->setEnabled(true);
            break;
        default:
            break;
        }
    }
    else if (ifOver == "Han")
    {
        QMessageBox::StandardButton result = QMessageBox::information(this, "胜负已分！",
                                                                     "恭喜黑方获胜！\n是否要重置对局？",
                                                                     QMessageBox::Yes | QMessageBox::No);
        switch (result)
        {
        case QMessageBox::Yes:
            Steps.s.clear();
            for (int i = 0; i < 32; i++)
            {
                allButton[i]->move(qiziCoordinate[i]);
            }
            ifOver = "none";
            chuhanRound = "none";
            resetRuleBase();
            PrechuhanRound = "none";
            MainWindow::on_Continue_clicked();
            if(autoNum == 3) on_pvp_radioButton_clicked();
            break;
        case QMessageBox::No:
            ui->Replay->setEnabled(true);
            break;
        default:
            break;
        }
    }
    chButton = nullptr;
    if (ifnRepeat) savStep(ry,rx,Steps);
}

void MainWindow::xiangqitimeEvent()
{
    if (autoNum == 0)
    {
        autoTimer->stop();
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
        //qDebug() << "准备观摩一场视觉盛宴吧！";
    }
    if(autoNum == 3)
    {
        if(autoSum < StepsBak.s.length())
        {
            analysisStep(StepsBak.s[autoSum].toStdString(),false);
        }
        else
        {
            ui->pvp_radioButton->setChecked(true);
            on_pvp_radioButton_clicked();
            ui->Replay->setEnabled(true);
            ui->Repent->setEnabled(true);
            StepsBak = AllST();
            ui->Replay->setText("回放");
        }
    }
if (autoSum == 1 && (ui->pve_radioButton->isChecked() || ui->eve_radioButton->isChecked()))
{
    autoTimer->stop();
    if (ui->pve_radioButton->isChecked())
    {
        difficulty = 7 - comboIndex * 2;
    }
    if (ui->eve_radioButton->isChecked())
    {
        difficulty = 9 - comboIndex / 2 * 4;
    }

    // First ask the cloudbook to adjudicate repetition rules (queryrule) using recent move history.
    // Then pass any "ban" (and optionally "draw") moves into queryall so the random selection won't pick illegal/undesired moves.
    auto startQueryAll = [this](const QString& banParam) {
        QUrl url("http://www.chessdb.cn/chessdb.php");
        QUrlQuery q;
        q.addQueryItem("action", "queryall");
        q.addQueryItem("board", getFEN());
        q.addQueryItem("showall", "1");
        if (!banParam.isEmpty())
        {
            q.addQueryItem("ban", banParam);
        }
        url.setQuery(q);
        request.setUrl(url);

        reply = tempManager->get(request);
        connect(reply, &QNetworkReply::finished, this, [this, r = reply.data()]() {
            // Ignore stale/cancelled replies (e.g. user started dragging pieces)
            if (reply.data() != r) {
                if (r) r->deleteLater();
                return;
            }
            if (!r) {
                reply = nullptr;
                return;
            }
            if (r->error() == QNetworkReply::NoError) {
                readInfo = r->readAll();
                ifReadyRead = true;
                // Prevent the next timer tick from re-sending a new request before consuming ifReadyRead
                autoSum = 0;
            } else {
                autoSum = 17;
            }
            r->deleteLater();
            reply = nullptr;
        });

        autoTimer->start(1000);
    };

    QString startFen;
    QString movelist;
    if (g_ruleEnable && buildQueryruleInputs(g_ruleTailMoves, startFen, movelist))
    {
        ui->label_3->setText("云库棋规裁定中...");
        QUrl url("http://www.chessdb.cn/chessdb.php");
        QUrlQuery q;
        q.addQueryItem("action", "queryrule");
        q.addQueryItem("board", startFen);
        q.addQueryItem("movelist", movelist);
        if (g_ruleRepTimes >= 1 && g_ruleRepTimes <= 10)
        {
            q.addQueryItem("reptimes", QString::number(g_ruleRepTimes));
        }
        url.setQuery(q);
        request.setUrl(url);

        reply = tempManager->get(request);
        connect(reply, &QNetworkReply::finished, this, [this, startQueryAll, r = reply.data()]() {
            if (reply.data() != r) {
                if (r) r->deleteLater();
                return;
            }
            QString banParam;
            if (r && r->error() == QNetworkReply::NoError)
            {
                banParam = banFromQueryruleResponse(r->readAll());
            }
            if (r) r->deleteLater();
            reply = nullptr;

            startQueryAll(banParam);
        });
    }
    else
    {
        startQueryAll(QString());
    }
}

    if (ifReadyRead)
    {
        autoTimer->stop();
        ifReadyRead = false;
        autoSum = 0;
        ui->label_3->setText("已成功连接至中国象棋云库！");
        if (readInfo.size() < 4) {
            // Avoid out-of-bounds access when the cloud returns an empty/short response
            autoSum = 17;
            ui->label_3->setText("云库返回异常：响应过短。");
            autoTimer->start(1000);
            return;
        }
        if (readInfo[0] == 'm' && readInfo[1] == 'o' && readInfo[2] == 'v')
        {
            QByteArrayList BAList;
            if(readInfo.contains('|'))
            {
                BAList = readInfo.split('|');
            }
            else
            {
                BAList.append(readInfo);
            }
            int one = rand() % 2 + 1;
            int t = 0;
            if(one == 1)
            {
                t = 0;
            }
            else if(BAList.length()>difficulty)
            {
                t = rand()%difficulty+1;
            }
            else if(BAList.length()>1)
            {
                t=rand()%(BAList.length() - 1);
            }
            else
            {
                t=0;
            }
            analysisStep(BAList[t].toStdString());
            autoTimer->start(1000);
        }
        else if (readInfo[0] == 'E')
        {
            autoTimer->stop();
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
            autoTimer->stop();
            ui->pvp_radioButton->setChecked(true);
            MainWindow::on_pvp_radioButton_clicked();
            autoNum = 0;
            QMessageBox box;
            box.setText("下的什么烂棋？和别人玩去吧。");
            box.exec();
        }
        else if ((readInfo[0] == 'u' && readInfo[1] == 'n') || (readInfo[0] == 'n' && readInfo[1] == 'o'))
        {
            autoTimer->stop();
            ui->pvp_radioButton->setChecked(true);
            MainWindow::on_pvp_radioButton_clicked();
            autoNum = 0;
            QMessageBox box;
            box.setText("你成功把电脑给整不会了，电脑不和你玩了。");
            box.exec();
        }
        else if ((readInfo[0] == 'c' && readInfo[1] == 'h') || (readInfo[0] == 's'))
        {
            autoTimer->stop();
            ui->pvp_radioButton->setChecked(true);
            MainWindow::on_pvp_radioButton_clicked();
            autoNum = 0;
            QMessageBox box;
            ui->Replay->setEnabled(true);
            box.setText("胜负已分！");
            box.exec();
        }
        else
        {
            autoTimer->stop();
            ui->pvp_radioButton->setChecked(true);
            MainWindow::on_pvp_radioButton_clicked();
            autoNum = 0;
            QMessageBox box;
            box.setText("出现错误，可能是因必要的组件缺失而导致。");
            box.exec();
        }
    }
    if (!ifReadyRead && autoSum > 16 && (ui->pve_radioButton->isChecked() || ui->eve_radioButton->isChecked()))
    {
        autoTimer->stop();
        ui->pvp_radioButton->setChecked(true);
        MainWindow::on_pvp_radioButton_clicked();
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


// ===== UI scaling helpers (Qt6, cross-platform) =====

bool MainWindow::isPieceButton(const QWidget* w) const
{
    for (int i = 0; i < 32; ++i)
    {
        if (allButton[i] == w) return true;
    }
    return false;
}

void MainWindow::captureBaseLayout()
{
    if (m_baseCaptured) return;

    // We cannot rely on ui->centralwidget->size() here because before the window is shown
    // some platforms report a very small size. Instead, infer the design coordinate system
    // from the authored geometries of direct children.
    int maxRight = 0;
    int maxBottom = 0;

    const auto directChildren = ui->centralwidget->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
    for (QWidget* w : directChildren)
    {
        const QRect g = w->geometry();
        m_baseGeom.insert(w, g);
        const int pt = w->font().pointSize();
        if (pt > 0) m_baseFontPt.insert(w, pt);

        maxRight = std::max(maxRight, g.x() + g.width());
        maxBottom = std::max(maxBottom, g.y() + g.height());
    }

    // Fallback: if the computed area is suspiciously small, use the original authoring size.
    if (maxRight < 200 || maxBottom < 200)
    {
        m_designCentralSize = QSize(1299, 796);
    }
    else
    {
        m_designCentralSize = QSize(maxRight, maxBottom);
    }

    m_baseCaptured = true;
}

void MainWindow::prepareScalableImages()
{
    // Scale the board background with the label geometry.
    if (ui->Qipan) ui->Qipan->setScaledContents(true);

    // Convert piece buttons from background-image to border-image so they scale with geometry.
    // (Qt styleSheet background-image does NOT scale by default.)
    for (int i = 0; i < 32; ++i)
    {
        if (!allButton[i]) continue;
        const QString ss = allButton[i]->styleSheet();
        static const QRegularExpression re(R"(background-image\s*:\s*url\(([^)]+)\)\s*;?)");
        const auto m = re.match(ss);
        if (m.hasMatch())
        {
            const QString url = m.captured(1).trimmed();
            allButton[i]->setStyleSheet(QString("border-image: url(%1) 0 0 0 0 stretch stretch; background: transparent; border: none;").arg(url));
        }
    }
}

void MainWindow::recomputeBoardCoordinates()
{
    if (!ui->Qipan) return;

    const QRect br = ui->Qipan->geometry();
    const int x0 = br.x();
    const int y0 = br.y();

    const int stepX = qMax(1, int(qRound(kBaseStepX * m_scaleX)));
    const int stepY = qMax(1, int(qRound(kBaseStepY * m_scaleY)));
    const int riverGap = qMax(0, int(qRound(kBaseRiverGap * m_scaleX)));

    for (int file = 0; file < 9; ++file)
    {
        const int y = y0 + file * stepY;
        for (int rank = 0; rank < 10; ++rank)
        {
            int x = 0;
            if (rank <= 4)
                x = x0 + rank * stepX;
            else
                x = x0 + 4 * stepX + riverGap + (rank - 5) * stepX;

            qipanCoordinates[file][rank] = QPoint(x, y);
        }
    }

    // Keep captured sentinel consistent with analysisStep() and queryrule reconstruction.
    kCapturedOffboard = capturedOffboardPoint();
}

void MainWindow::updateInitialPieceCoords()
{
    ensureInitCells();
    for (int i = 0; i < 32; ++i)
    {
        if (g_initFile[i] >= 0 && g_initRank[i] >= 0)
            qiziCoordinate[i] = qipanCoordinates[g_initFile[i]][g_initRank[i]];
        else
            qiziCoordinate[i] = capturedOffboardPoint();
    }
}

QVector<MainWindow::PieceCell> MainWindow::snapshotPieceCells() const
{
    QVector<PieceCell> cells;
    cells.resize(32);

    auto nearestCell = [&](const QPoint& pt, int& outFile, int& outRank) -> bool
    {
        qint64 best = std::numeric_limits<qint64>::max();
        int bf = 0, br = 0;
        for (int f = 0; f < 9; ++f)
        {
            for (int r = 0; r < 10; ++r)
            {
                const QPoint gp = qipanCoordinates[f][r];
                const qint64 dx = pt.x() - gp.x();
                const qint64 dy = pt.y() - gp.y();
                const qint64 d = dx * dx + dy * dy;
                if (d < best)
                {
                    best = d;
                    bf = f;
                    br = r;
                }
            }
        }
        outFile = bf;
        outRank = br;
        return true;
    };

    for (int i = 0; i < 32; ++i)
    {
        if (!allButton[i]) { cells[i].captured = true; continue; }
        const QPoint p(allButton[i]->x(), allButton[i]->y());
        if (p.x() < -1000 || p.y() < -1000)
        {
            cells[i].captured = true;
            continue;
        }

        // Exact match first (fast path).
        bool found = false;
        for (int f = 0; f < 9 && !found; ++f)
        {
            for (int r = 0; r < 10; ++r)
            {
                if (p == qipanCoordinates[f][r])
                {
                    cells[i].file = f;
                    cells[i].rank = r;
                    cells[i].captured = false;
                    found = true;
                    break;
                }
            }
        }
        if (!found)
        {
            int f = 0, r = 0;
            nearestCell(p, f, r);
            cells[i].file = f;
            cells[i].rank = r;
            cells[i].captured = false;
        }
    }
    return cells;
}

void MainWindow::restorePieceCells(const QVector<PieceCell>& cells)
{
    for (int i = 0; i < 32 && i < cells.size(); ++i)
    {
        if (!allButton[i]) continue;
        if (cells[i].captured)
        {
            allButton[i]->move(capturedOffboardPoint());
        }
        else if (cells[i].file >= 0 && cells[i].file < 9 && cells[i].rank >= 0 && cells[i].rank < 10)
        {
            allButton[i]->move(qipanCoordinates[cells[i].file][cells[i].rank]);
        }
    }
}

void MainWindow::applyScaledLayout()
{
    if (!m_baseCaptured) return;

    const int cw = ui->centralwidget->width();
    const int ch = ui->centralwidget->height();

    double sx = (m_designCentralSize.width() > 0) ? (double(cw) / double(m_designCentralSize.width())) : 1.0;
    double sy = (m_designCentralSize.height() > 0) ? (double(ch) / double(m_designCentralSize.height())) : 1.0;

    if (g_uiKeepAspect)
    {
        const double s = std::min(sx, sy);
        sx = s;
        sy = s;
    }

    const int dx = int(qRound((cw - m_designCentralSize.width() * sx) / 2.0));
    const int dy = int(qRound((ch - m_designCentralSize.height() * sy) / 2.0));

    m_scaleX = sx;
    m_scaleY = sy;
    m_offset = QPoint(dx, dy);

    const double fontScale = std::min(sx, sy);

    // Scale all direct children of centralwidget using their design geometries.
    for (auto it = m_baseGeom.constBegin(); it != m_baseGeom.constEnd(); ++it)
    {
        QWidget* w = it.key();
        if (!w) continue;

        if (isPieceButton(w))
            continue; // handled separately

        const QRect b = it.value();
        const QRect nr(
            dx + int(qRound(b.x() * sx)),
            dy + int(qRound(b.y() * sy)),
            qMax(1, int(qRound(b.width() * sx))),
            qMax(1, int(qRound(b.height() * sy)))
        );
        w->setGeometry(nr);

        if (m_baseFontPt.contains(w))
        {
            QFont f = w->font();
            f.setPointSizeF(qMax(1.0, m_baseFontPt.value(w) * fontScale));
            w->setFont(f);
        }
    }

    // Piece size scales with overall scale (keep aspect).
    m_pieceSize = qMax(1, int(qRound(kBasePieceSize * fontScale)));
    for (int i = 0; i < 32; ++i)
    {
        if (!allButton[i]) continue;
        allButton[i]->setFixedSize(m_pieceSize, m_pieceSize);
    }

    // Update board grid coordinates and initial placement map.
    recomputeBoardCoordinates();
    updateInitialPieceCoords();
}

// ===== end UI scaling helpers =====

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Create QObjects after QApplication is initialized (Qt6, cross-platform safe)
    autoTimer = new QTimer(this);
    tempManager = new QNetworkAccessManager(this);
    reply = nullptr;
    /*
    QImage bgimg;
    bgimg.load(QString(":/Img/Img/Qipan.png"));
    ui->Qipan->setPixmap(QPixmap::fromImage(bgimg));
    */
    // UI scale/board coords will be computed in applyScaledLayout().
    ensureInitCells();
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
    //目前没找到把所有棋子对象统一管理的更好办法...
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
    connect(autoTimer, &QTimer::timeout, this, &MainWindow::xiangqitimeEvent);
    ui->pvp_radioButton->setChecked(true);
    ui->Continue->setEnabled(false);
    ui->Replay->setEnabled(false);
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::myabout);


// Load cloudbook settings (cross-platform, Qt6). Stored per-user.
{
    QSettings s("Xiangqi197", "Xiangqi");
    g_ruleEnable = s.value("cloudbook/enableQueryrule", true).toBool();
    g_ruleTailMoves = s.value("cloudbook/ruleTailMoves", 12).toInt();
    if (g_ruleTailMoves < 4) g_ruleTailMoves = 4;
    g_ruleRepTimes = s.value("cloudbook/reptimes", 1).toInt();
    if (g_ruleRepTimes < 1) g_ruleRepTimes = 1;
    if (g_ruleRepTimes > 10) g_ruleRepTimes = 10;
    g_ruleAvoidDraw = s.value("cloudbook/avoidDraw", false).toBool();
}
// NOTE: Do NOT connect this action manually.
// Qt Designer's connectSlotsByName will auto-connect
// actionCloudbookSettings::triggered() -> on_actionCloudbookSettings_triggered().
// If we connect again here, the slot will run twice and the dialog appears twice.




// ----- UI scaling init -----
captureBaseLayout();
prepareScalableImages();

// Apply desired startup size (edit g_uiStartWidth / g_uiStartHeight at top of this file).
if (g_uiStartWidth > 0 && g_uiStartHeight > 0)
{
    this->resize(g_uiStartWidth, g_uiStartHeight);
}

// IMPORTANT: Do NOT call applyScaledLayout() here.
// Before the window is shown, some platforms report a very small centralwidget size,
// which would make the scale factor enormous ("one piece fills the window").
// We defer the first scaling pass to showEvent() (via a 0ms singleShot).

}

MainWindow::~MainWindow()
{
    if (reply) {
        reply->abort();
        reply->deleteLater();
        reply = nullptr;
    }
    if (autoTimer) {
        autoTimer->stop();
        // autoTimer has parent 'this' and will be deleted automatically
        autoTimer = nullptr;
    }
    // tempManager has parent 'this' and will be deleted automatically
    tempManager = nullptr;
    delete ui;
}


void MainWindow::showEvent(QShowEvent* event)
{
    QMainWindow::showEvent(event);

    if (m_firstShowApplied) return;

    // Defer the very first scaling pass to the next event loop turn so that
    // centralwidget and its children have their final, platform-correct sizes.
    QTimer::singleShot(0, this, [this]() {
        if (m_firstShowApplied) return;
        captureBaseLayout();
        applyScaledLayout();
        for (int i = 0; i < 32; ++i)
        {
            if (allButton[i]) allButton[i]->move(qiziCoordinate[i]);
        }
        // Record the starting position for cloudbook queryrule after the UI/pieces are placed.
        resetRuleBase();
        m_firstShowApplied = true;
    });
}


void MainWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);

    if (!m_baseCaptured) return;

    // Avoid fighting with dragging.
    if (ifPressed) return;

    // Until the first show/layout pass completes (showEvent singleShot), do not attempt
    // to scale here; doing so can use an incorrect (tiny) centralwidget size on some platforms.
    if (!m_firstShowApplied) return;

    const auto cells = snapshotPieceCells();
    applyScaledLayout();
    restorePieceCells(cells);
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
    QMouseEvent *mouseEvent = nullptr;
    if (event->type() == QEvent::MouseButtonPress ||
        event->type() == QEvent::MouseButtonRelease ||
        event->type() == QEvent::MouseMove)
    {
        mouseEvent = static_cast<QMouseEvent *>(event);
    }
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
        autoTimer->stop();
        if(reply) {reply->abort();
            reply->deleteLater();
            reply=nullptr;}
        autoSum = 0;
    }
    for (int B = 0; B < 32; B++)
    {
        if (obj == allButton[B])
        {
            if (event->type() == QEvent::MouseButtonPress)
            {
                if (!mouseEvent) return true;
                chButton = allButton[B];
                ifPressed = true;
                chButton->raise();
                if (!CheckRoundCorrectAndSetRound())
                {
                    return true;
                }
                cStep = preStep(chButton, QPoint(chButton->x(), chButton->y()));
                relaPoint = QPoint(mouseEvent->x(), mouseEvent->y());
                return true;
            }
            else if (event->type() == QEvent::MouseMove && ifPressed)
            {
                if (!mouseEvent) return true;
                chButton->move(mouseEvent->x() + chButton->x() - relaPoint.x(), mouseEvent->y() + chButton->y() - relaPoint.y());
                return true;
            }
            break;
        }
    }
    /*
    这一部分原本的内容已经被删了，上面的循环是其代替实现内容。
    */
    if (event->type() == QEvent::MouseButtonRelease)
    {
        autoTimer->start(1000);
        if (chButton == nullptr || cStep.preButton == nullptr)
        {
            return true;
        }
        ifPressed = false;
        int rx = 0, ry = 0;
        // Snap to the nearest board intersection using the current scaled grid.
        {
            qint64 best = std::numeric_limits<qint64>::max();
            int bf = 0, br = 0;
            const QPoint pt(chButton->x(), chButton->y());
            for (int f = 0; f < 9; ++f)
            {
                for (int r = 0; r < 10; ++r)
                {
                    const QPoint gp = qipanCoordinates[f][r];
                    const qint64 dx = pt.x() - gp.x();
                    const qint64 dy = pt.y() - gp.y();
                    const qint64 d = dx * dx + dy * dy;
                    if (d < best)
                    {
                        best = d;
                        bf = f;
                        br = r;
                    }
                }
            }
            ry = bf; // file (0..8)
            rx = br; // rank (0..9)
        }
        // Determine the starting cell (file/rank) from the original point.
        // Using nearest-cell here makes the logic robust to rounding / scaling.
        int rh = 0; // start file
        int rw = 0; // start rank
        {
            qint64 best = std::numeric_limits<qint64>::max();
            for (int f = 0; f < 9; ++f)
            {
                for (int r = 0; r < 10; ++r)
                {
                    const QPoint gp = qipanCoordinates[f][r];
                    const qint64 dx = cStep.prePoint.x() - gp.x();
                    const qint64 dy = cStep.prePoint.y() - gp.y();
                    const qint64 d = dx * dx + dy * dy;
                    if (d < best)
                    {
                        best = d;
                        rh = f;
                        rw = r;
                    }
                }
            }
        }
        int pWidth, pHeight;
        pWidth = rx - rw;
        pHeight = ry - rh;
        if (pWidth == 0 && pHeight == 0)
        {
            cStep.preButton->move(qipanCoordinates[rh][rw]);
            cStep = preStep();
            chuhanRound = PrechuhanRound;
            return true;
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
                                return true;
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
                                return true;
                            }
                        }
                    }
                }
            }
            else
            {
                ShowfBoxAndQuash("您的落点是错误的，车只能直行。");
                return true;
            }
        }
        if (chButton == allButton[2] || chButton == allButton[3] || chButton == allButton[18] || chButton == allButton[19])
        {

            if (!((absoluteValue(pWidth) == 2 && absoluteValue(pHeight) == 1) || (absoluteValue(pWidth) == 1 && absoluteValue(pHeight) == 2)))
            {
                ShowfBoxAndQuash("您的落点是错误的，马只能走日。");
                return true;
            }
            else if (absoluteValue(pWidth) == 2)
            {
                for (int i = 0; i < 32; i++)
                {
                    if (QPoint(allButton[i]->x(), allButton[i]->y()) == qipanCoordinates[rh][rw + pWidth / absoluteValue(pWidth)])
                    {
                        ShowfBoxAndQuash("您的落点是错误的，拌马脚了。");
                        return true;
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
                        return true;
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
                    return true;
                }
            }
            else if (chButton == allButton[20] || chButton == allButton[21])
            {
                if (rx <= 4)
                {
                    ShowfBoxAndQuash("您的落点是错误的，相不能越界。");
                    return true;
                }
            }

            if (!(absoluteValue(pWidth) == 2 && absoluteValue(pHeight) == 2))
            {
                ShowfBoxAndQuash("您的落点是错误的，相只能走田。");
                return true;
            }
            for (int i = 0; i < 32; i++)
            {
                if (QPoint(allButton[i]->x(), allButton[i]->y()) == qipanCoordinates[rh + pHeight / absoluteValue(pHeight)][rw + pWidth / absoluteValue(pWidth)])
                {
                    ShowfBoxAndQuash("您的落点是错误的，塞相眼了。");
                    return true;
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
                    return true;
                }
            }
            else if (chButton == allButton[22] || chButton == allButton[23])
            {
                if (rx < 7 || ry < 3 || ry > 5)
                {
                    ShowfBoxAndQuash("您的落点是错误的，士不能出格。");
                    return true;
                }
            }
            if (absoluteValue(pWidth) != 1 || absoluteValue(pHeight) != 1)
            {
                ShowfBoxAndQuash("您的落点是错误的，士只能走斜线。");
                return true;
            }
        }
        if (chButton == allButton[8] || chButton == allButton[24])
        {
            if (chButton == allButton[8])
            {
                if (rx > 2 || ry < 3 || ry > 5)
                {
                    ShowfBoxAndQuash("您的落点是错误的，将不能出格。");
                    return true;
                }
            }
            if (chButton == allButton[24])
            {
                if (rx < 7 || ry < 3 || ry > 5)
                {
                    ShowfBoxAndQuash("您的落点是错误的，将不能出格。");
                    return true;
                }
            }
            if (!(absoluteValue(pWidth) == 0 && absoluteValue(pHeight) == 1) && !(absoluteValue(pWidth) == 1 && absoluteValue(pHeight) == 0))
            {
                ShowfBoxAndQuash("您的落点是错误的，将只能直走一格。");
                return true;
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
                        return true;
                    }
                    if (ifexq == 1 && !iffinexq)
                    {
                        ShowfBoxAndQuash("您的落点是错误的，炮不能只翻过棋子而不吃棋子。");
                        return true;
                    }
                    if (!ifexq && iffinexq)
                    {
                        ShowfBoxAndQuash("您的落点是错误的，炮不能不翻过棋子而吃掉棋子。");
                        return true;
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
                        return true;
                    }
                    if (ifexq == 1 && !iffinexq)
                    {
                        ShowfBoxAndQuash("您的落点是错误的，炮不能只翻过棋子而不吃棋子。");
                        return true;
                    }
                    if (!ifexq && iffinexq)
                    {
                        ShowfBoxAndQuash("您的落点是错误的，炮不能不翻过棋子而吃掉棋子。");
                        return true;
                    }
                }
            }
            else
            {
                ShowfBoxAndQuash("您的落点是错误的，炮只能直行。");
                return true;
            }
        }
        if (chButton == allButton[11] || chButton == allButton[12] || chButton == allButton[13] || chButton == allButton[14] || chButton == allButton[15] || chButton == allButton[27] || chButton == allButton[28] || chButton == allButton[29] || chButton == allButton[30] || chButton == allButton[31])
        {
            if (chButton == allButton[11] || chButton == allButton[12] || chButton == allButton[13] || chButton == allButton[14] || chButton == allButton[15])
            {
                if (rx <= 4 && absoluteValue(pHeight))
                {
                    ShowfBoxAndQuash("您的落点是错误的，兵在过河前不能左右行动。");
                    return true;
                }
                if (pWidth < 0)
                {
                    ShowfBoxAndQuash("您的落点是错误的，兵在任何情况下不能后退。");
                    return true;
                }
            }
            if (chButton == allButton[27] || chButton == allButton[28] || chButton == allButton[29] || chButton == allButton[30] || chButton == allButton[31])
            {
                if (rx > 4 && absoluteValue(pHeight))
                {
                    ShowfBoxAndQuash("您的落点是错误的，兵在过河前不能左右行动。");
                    return true;
                }
                if (pWidth > 0)
                {
                    ShowfBoxAndQuash("您的落点是错误的，兵在任何情况下不能后退。");
                    return true;
                }
            }
            if (!(absoluteValue(pWidth) == 0 && absoluteValue(pHeight) == 1) && !(absoluteValue(pWidth) == 1 && absoluteValue(pHeight) == 0))
            {
                ShowfBoxAndQuash("您的落点是错误的，兵只能直走一格。");
                return true;
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
                        return true;
                    }
                    else if (allButton[s] == chButton && qiziIsChuOrHan[s] != qiziIsChuOrHan[i])
                    {
                        break;
                    }
                }
                yiqvshiStep = preStep(allButton[i], QPoint(allButton[i]->x(), allButton[i]->y()));
                allButton[i]->move(capturedOffboardPoint());
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
                return true;
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
            QMessageBox::StandardButton result = QMessageBox::information(this, "胜负已分！", "恭喜红方获胜！\n是否要重置对局？", QMessageBox::Yes | QMessageBox::No);
            switch (result)
            {
            case QMessageBox::Yes:
                for (int i = 0; i < 32; i++)
                {
                    allButton[i]->move(qiziCoordinate[i]);
                }
                ifOver = "none";
                chuhanRound = "none";
                Steps.s.clear();
                resetRuleBase();
                PrechuhanRound = "none";
                MainWindow::on_Continue_clicked();
                if(autoNum == 3) on_pvp_radioButton_clicked();
                break;
            case QMessageBox::No:
                ui->Replay->setEnabled(true);
                break;
            default:
                break;
            }
        }
        else if (ifOver == "Han")
        {
            QMessageBox::StandardButton result = QMessageBox::information(this, "胜负已分！", "恭喜黑方获胜！\n是否要重置对局？", QMessageBox::Yes | QMessageBox::No);
            switch (result)
            {
            case QMessageBox::Yes:
                for (int i = 0; i < 32; i++)
                {
                    allButton[i]->move(qiziCoordinate[i]);
                }
                ifOver = "none";
                chuhanRound = "none";
                Steps.s.clear();
                resetRuleBase();
                PrechuhanRound = "none";
                MainWindow::on_Continue_clicked();
                if(autoNum == 3) on_pvp_radioButton_clicked();
                break;
            case QMessageBox::No:
                ui->Replay->setEnabled(true);
                break;
            default:
                break;
            }
        }
        chButton = nullptr;
        savStep(ry, rx,Steps);
        return true;
    }
    return false;
}

void MainWindow::on_Start_clicked()
{
    QMessageBox::StandardButton result = QMessageBox::information(this, "开始", "是否要重置对局？", QMessageBox::Yes | QMessageBox::No);
    switch (result)
    {
    case QMessageBox::Yes:
        ui->Replay->setEnabled(false);
        ui->Replay->setText("回放");
        autoNum=saveNum;
        if(reply){
            reply->abort();
            reply->deleteLater();
            reply=nullptr;}
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
        Steps.s.clear();
        resetRuleBase();
        StepsBak = AllST();
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
    autoTimer->stop();
    if(reply){
        reply->abort();
        reply->deleteLater();
        reply=nullptr;}
    SumBak = autoSum;
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
        autoTimer->start(1000);
    }
    if (autoNum == 3){
        autoSum = SumBak;
        autoTimer->start(ti*1000);
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
    if (!Steps.s.length())
    {
        QMessageBox huiqiBox;
        huiqiBox.setText("您无棋可悔");
        huiqiBox.exec();
        return;
    }
    if(!Pause && (ui->pve_radioButton->isChecked() || ui->eve_radioButton->isChecked()))
    {
        QMessageBox huiqiPBox;
        huiqiPBox.setText("在交战双方存在电脑时，请先点击暂停再悔棋");
        huiqiPBox.exec();
        return;
    }
    MainWindow::setEnabled(false);
    if(reply) {reply->abort();
    reply->deleteLater();
        reply=nullptr;}
    autoTimer->stop();
    autoSum = 0;
    ifReadyRead = false;
    const int s = Steps.s.length() - 1;
    const QString stepOrig = Steps.s[s];
    if (stepOrig.size() < 9 || !stepOrig.startsWith("move:"))
    {
        QMessageBox box;
        box.setText("悔棋出错：步法记录损坏。");
        box.exec();
        MainWindow::setEnabled(true);
        return;
    }

    // Swap <from> and <to> to revert the last move
    QString step = stepOrig;
    QChar t = step[7];
    QChar r = step[8];
    step[7] = step[5];
    step[8] = step[6];
    step[5] = t;
    step[6] = r;

    analysisStep(step.toStdString(), false);

    // Restore captured piece (if any)
    if (step.size() >= 11 && step[9] != QChar('N'))
    {
        bool ok = false;
        const int capIdx = step.mid(9, 2).toInt(&ok);
        const int fx = charToqipanInt(step[5].toLatin1());
        const int fy = 9 - QString(step[6]).toInt();
        if (ok && capIdx >= 0 && capIdx < 32 && fx >= 0 && fx < 9 && fy >= 0 && fy < 10)
        {
            allButton[capIdx]->move(qipanCoordinates[fx][fy]);
        }
    }

    Steps.s.removeAt(s);
    if (chuhanRound == "Chu")
    {
        chuhanRound = "Han";
    }
    else
    {
        chuhanRound = "Chu";
    }
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
    ui->Replay->setText("回放");
    ui->Replay->setEnabled(false);
    //StepsBak = AllST();
    autoTimer->stop();
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
    if(reply){
        reply->abort();
    reply->deleteLater();
        reply=nullptr;
    }
    autoSum = 0;
    ifReadyRead = false;
    if(repl)
    {
        Steps.s = Steps.s_tmp;
        Steps.s_tmp.clear();
        repl = false;
    }
    MainWindow::setEnabled(true);
    ui->label_3->setText("切换完毕！");
    saveNum = autoNum;
    if (!Pause)
    {
        autoTimer->start(1000);
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
    ui->Replay->setText("回放");
    autoNum = 2;
    saveNum = autoNum;
    //StepsBak = AllST();
    ui->label_3->setText("正在执行切换，请耐心等待...");
    ui->Replay->setEnabled(false);
    MainWindow::setEnabled(false);
    if(repl)
    {
        Steps.s = Steps.s_tmp;
        Steps.s_tmp.clear();
        repl = false;
    }
    if (reply){
        reply->abort();
    reply->deleteLater();
        reply=nullptr;}
    autoSum = 0;
    ifReadyRead = false;
    MainWindow::setEnabled(true);
    ui->label_3->setText("切换完毕！");
    if (!Pause)
    {
        autoTimer->start(1000);
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
    autoTimer->stop();
    if(reply){
        reply->abort();
    reply->deleteLater();
        reply=nullptr;}
    ifReadyRead = false;
    autoSum = 0;
    if (!fs::exists(saveDir))
    {
        fs::create_directory(saveDir);
    }
    else
    {
        //qDebug() << "Existed!Pass!";
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
    fout << chuhanRound.toStdString() << "\n";
    for (int i = 0; i < Steps.s.length(); i++)
    {
        fout << Steps.s[i].toStdString() << "\n";
    }
    fout.close();
    if (!Pause)
    {
        autoTimer->start(1000);
    }
    return;
}

void MainWindow::on_Load_clicked()
{
    autoTimer->stop();
    if(reply){
        reply->abort();
    reply->deleteLater();
        reply=nullptr;}
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
    for (int i = 0; i < 32; i++)
    {
        allButton[i]->move(qiziCoordinate[i]);
    }
    Steps = AllST();
    StepsBak = AllST();
    while (getline(fin, str))
    {
        if (a > 0)
        {
            analysisStep(str);
        }
        else if (a == 0)
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

            // The move list in the file starts from this position, so use it as queryrule base.
            resetRuleBase();
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
        autoTimer->start(1000);
    }
    //qDebug() << Steps.s << "\n" << "and";
    //qDebug() << Steps.s_tmp;
    return;
}

void MainWindow::on_pvp_radioButton_clicked()
{
    ui->Replay->setEnabled(false);
    autoSum = 0;
    autoNum = 0;
    if(repl)
    {
        Steps.s = Steps.s_tmp;
        Steps.s_tmp.clear();
        repl = false;
        //qDebug() << Steps.s << "!!!";
    }
    autoTimer->stop();
    ui->Replay->setText("回放");
    ui->label_3->setText("正在执行切换，请耐心等待...");
    saveNum = autoNum;
    //StepsBak = AllST();
    MainWindow::setEnabled(false);
    if(reply){
        reply->abort();
    reply->deleteLater();
        reply=nullptr;}
    ifReadyRead = false;
    MainWindow::setEnabled(true);
    ui->label_3->setText("切换完毕！");
}

void MainWindow::myabout()
{
    QMessageBox aboBox;
    aboBox.setText("Made by Chen-197\nLicense: GPL-3.0");
    aboBox.exec();
}


void MainWindow::on_actionCloudbookSettings_triggered()
{
    QDialog dlg(this);
    dlg.setWindowTitle("云库设置");
    QFormLayout* form = new QFormLayout(&dlg);

    QCheckBox* cbEnable = new QCheckBox("启用棋规裁定（queryrule）", &dlg);
    cbEnable->setChecked(g_ruleEnable);

    QSpinBox* sbTail = new QSpinBox(&dlg);
    sbTail->setRange(4, 200);
    sbTail->setValue(g_ruleTailMoves);
    sbTail->setToolTip("发送给 queryrule 的最近历史着法步数（至少 4 步）。");

    QSpinBox* sbRep = new QSpinBox(&dlg);
    sbRep->setRange(1, 10);
    sbRep->setValue(g_ruleRepTimes);
    sbRep->setToolTip("reptimes：从第几次循环开始裁定（1~10）。");

    QCheckBox* cbAvoidDraw = new QCheckBox("避免和棋着法（将 rule:draw 也视为 ban）", &dlg);
    cbAvoidDraw->setChecked(g_ruleAvoidDraw);

    form->addRow(cbEnable);
    form->addRow("历史步数（movelist）:", sbTail);
    form->addRow("reptimes:", sbRep);
    form->addRow(cbAvoidDraw);

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    form->addRow(buttons);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() == QDialog::Accepted)
    {
        g_ruleEnable = cbEnable->isChecked();
        g_ruleTailMoves = sbTail->value();
        if (g_ruleTailMoves < 4) g_ruleTailMoves = 4;
        g_ruleRepTimes = sbRep->value();
        if (g_ruleRepTimes < 1) g_ruleRepTimes = 1;
        if (g_ruleRepTimes > 10) g_ruleRepTimes = 10;
        g_ruleAvoidDraw = cbAvoidDraw->isChecked();

        QSettings s("Xiangqi197", "Xiangqi");
        s.setValue("cloudbook/enableQueryrule", g_ruleEnable);
        s.setValue("cloudbook/ruleTailMoves", g_ruleTailMoves);
        s.setValue("cloudbook/reptimes", g_ruleRepTimes);
        s.setValue("cloudbook/avoidDraw", g_ruleAvoidDraw);

        ui->label_3->setText(QString("云库设置已保存：%1，步数=%2，reptimes=%3%4")
                             .arg(g_ruleEnable ? "启用裁定" : "关闭裁定")
                             .arg(g_ruleTailMoves)
                             .arg(g_ruleRepTimes)
                             .arg(g_ruleAvoidDraw ? "，避和棋" : ""));
    }
}
void MainWindow::on_Replay_clicked()
{
    autoTimer->stop();
    QStringList items = {"0.1","0.2","0.4","1.0","2.0","2.5","4.0"};
    QString item = QInputDialog::getItem(this, "回放", "请选择时钟周期，单位：秒", items, 0, false);
    double t;
    if(item!="")t=item.toDouble();else return;
    for (int i = 0; i < 32; i++)
    {
        allButton[i]->move(qiziCoordinate[i]);
    }
    ifOver = "none";
    chuhanRound = "none";
    PrechuhanRound = "none";
    ui->pvp_radioButton->setChecked(true);
    on_pvp_radioButton_clicked();
    ui->Replay->setText("回放中");
    if (Steps.s.length()){
        StepsBak = Steps;
        //qDebug() << "456";
    }
    repl = true;
    ui->Replay->setEnabled(false);
    ui->Repent->setEnabled(false);
    autoSum = 0;
    autoNum = 3;
    autoTimer->start(1000*t);
    ti = t;
}
