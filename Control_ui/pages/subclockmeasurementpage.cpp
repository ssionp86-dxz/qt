#include "subclockmeasurementpage.h"

#include <QAbstractItemView>
#include <QBrush>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileDialog>
#include <QFrame>
#include <QGridLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLinearGradient>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QProgressBar>
#include <QPushButton>
#include <QRandomGenerator>
#include <QSaveFile>
#include <QStyle>
#include <QTabBar>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextStream>
#include <QVBoxLayout>
#include <QtMath>

#include <algorithm>
#include <cmath>
#include <functional>

namespace {

const int kRackCount = 6;
const int kRackRows = 5;
const int kRackColumns = 9;
const int kRackPointCount = kRackRows * kRackColumns;

int slotIndex(int row, int col)
{
    return row * kRackColumns + col;
}

QString slotPointText(int row, int col, int span)
{
    if (span <= 1) {
        return QString::fromUtf8("第%1排 / 第%2挂点")
                .arg(row + 1)
                .arg(col + 1);
    }

    return QString::fromUtf8("第%1排 / 第%2-%3挂点")
            .arg(row + 1)
            .arg(col + 1)
            .arg(col + span);
}

QString shapeText(SubClockShape shape)
{
    return shape == SubClockShape::RoundClock
            ? QString::fromUtf8("圆钟")
            : QString::fromUtf8("方钟");
}

QString stateText(SubClockState state)
{
    switch (state) {
    case SubClockState::Pending:
        return QString::fromUtf8("待测试");
    case SubClockState::Passed:
        return QString::fromUtf8("测试合格");
    case SubClockState::Failed:
        return QString::fromUtf8("测试不合格");
    case SubClockState::Uninstalled:
    default:
        return QString::fromUtf8("未安装");
    }
}

QColor stateFillColor(SubClockState state)
{
    switch (state) {
    case SubClockState::Pending:
        return QColor(QStringLiteral("#3b82f6"));
    case SubClockState::Passed:
        return QColor(QStringLiteral("#22c55e"));
    case SubClockState::Failed:
        return QColor(QStringLiteral("#ef4444"));
    case SubClockState::Uninstalled:
    default:
        return QColor(QStringLiteral("#7b8798"));
    }
}

QColor stateBorderColor(SubClockState state)
{
    switch (state) {
    case SubClockState::Pending:
        return QColor(QStringLiteral("#93c5fd"));
    case SubClockState::Passed:
        return QColor(QStringLiteral("#86efac"));
    case SubClockState::Failed:
        return QColor(QStringLiteral("#fca5a5"));
    case SubClockState::Uninstalled:
    default:
        return QColor(QStringLiteral("#cbd5e1"));
    }
}


QTime decorativeClockTime(const SubClockInfo &clock)
{
    const int seed = (clock.rackIndex + 1) * 97 + (clock.indexInRack + 3) * 17;
    const int hour = (seed / 13) % 12;
    const int minute = (seed * 7) % 60;
    return QTime(hour, minute, 0);
}

void drawClockHands(QPainter &painter, const QRectF &rect, const QColor &color,
                    const QTime &time, qreal borderWidth = 1.4)
{
    const QPointF c = rect.center();
    const qreal radius = qMin(rect.width(), rect.height()) * 0.5;

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);

    QPen tickPen(color, qMax<qreal>(1.0, borderWidth * 0.8));
    tickPen.setCapStyle(Qt::RoundCap);
    painter.setPen(tickPen);

    auto endpoint = [&](qreal lengthRatio, qreal degrees) {
        const qreal rad = qDegreesToRadians(degrees - 90.0);
        return QPointF(c.x() + std::cos(rad) * radius * lengthRatio,
                       c.y() + std::sin(rad) * radius * lengthRatio);
    };

    const qreal hourAngle = (time.hour() % 12 + time.minute() / 60.0) * 30.0;
    const qreal minuteAngle = time.minute() * 6.0;

    QPen hourPen(color, qMax<qreal>(1.5, borderWidth * 1.35));
    hourPen.setCapStyle(Qt::RoundCap);
    painter.setPen(hourPen);
    painter.drawLine(c, endpoint(0.7, hourAngle));

    QPen minutePen(color, qMax<qreal>(1.2, borderWidth));
    minutePen.setCapStyle(Qt::RoundCap);
    painter.setPen(minutePen);
    painter.drawLine(c, endpoint(0.9, minuteAngle));

    painter.setBrush(color);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(c, qMax<qreal>(2.2, borderWidth * 1.5), qMax<qreal>(2.2, borderWidth * 1.5));
    painter.restore();
}

void drawClockSymbol(QPainter &painter, const QRectF &rect, bool roundShape, const SubClockInfo *clock,
                     const QColor &lineColor, qreal borderWidth = 1.4)
{
    QPainterPath path;
    if (roundShape) {
        path.addEllipse(rect);
    } else {
        path.addRoundedRect(rect, qMin(rect.width(), rect.height()) * 0.16,
                            qMin(rect.width(), rect.height()) * 0.16);
    }
    painter.save();
    painter.setClipPath(path);
    drawClockHands(painter, rect, lineColor,
                   clock ? decorativeClockTime(*clock) : QTime(10, 10, 0),
                   borderWidth);
    painter.restore();
}

QString offsetText(const SubClockInfo &clock)
{
    if (!clock.active || clock.state == SubClockState::Pending || clock.state == SubClockState::Uninstalled) {
        return QString::fromUtf8("--");
    }
    return QString::number(clock.offsetMs, 'f', 3);
}

QString timeText(const SubClockInfo &clock)
{
    return clock.lastMeasured.isValid()
            ? clock.lastMeasured.toString(QStringLiteral("yyyy-MM-dd hh:mm:ss"))
            : QString::fromUtf8("--");
}
/*
void restyleWidget(QWidget *widget)
{
    if (!widget) {
        return;
    }
    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
    widget->update();
}
*/
QPushButton *createActionButton(const QString &text, bool primary, QWidget *parent,
                                const QString &objectName = QString())
{
    QPushButton *button = new QPushButton(text, parent);
    button->setCursor(Qt::PointingHandCursor);
    button->setProperty("actionRole", primary ? QStringLiteral("primary")
                                              : QStringLiteral("secondary"));
    if (!objectName.isEmpty()) {
        button->setObjectName(objectName);
    }
    return button;
}

QFrame *createMetricTile(const QString &title, const QString &value, QLabel **valueOut, QWidget *parent)
{
    QFrame *tile = new QFrame(parent);
    tile->setObjectName(QStringLiteral("metricTile"));

    QVBoxLayout *layout = new QVBoxLayout(tile);
    layout->setContentsMargins(14, 12, 14, 12);
    layout->setSpacing(6);

    QLabel *caption = new QLabel(title, tile);
    caption->setObjectName(QStringLiteral("metricCaption"));

    QLabel *valueLabel = new QLabel(value, tile);
    valueLabel->setObjectName(QStringLiteral("metricValue"));
    valueLabel->setWordWrap(true);

    layout->addWidget(caption);
    layout->addWidget(valueLabel);

    if (valueOut) {
        *valueOut = valueLabel;
    }
    return tile;
}

QWidget *createLegendItem(const QString &text,
                          const QColor &fillColor,
                          const QColor &borderColor,
                          bool roundShape,
                          QWidget *parent)
{
    QWidget *item = new QWidget(parent);
    QHBoxLayout *layout = new QHBoxLayout(item);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);
    QLabel *icon = new QLabel(item);
    if (roundShape) {
        icon->setFixedSize(34, 34);
        icon->setStyleSheet(QStringLiteral(
            "background:%1;"
            "border:1px solid %2;"
            "border-radius:17px;").arg(fillColor.name(), borderColor.name()));
    } else {
        icon->setFixedSize(52, 32);
        icon->setStyleSheet(QStringLiteral(
            "background:%1;"
            "border:1px solid %2;"
            "border-radius:8px;").arg(fillColor.name(), borderColor.name()));
    }

    QLabel *label = new QLabel(text, item);
    label->setObjectName(QStringLiteral("legendText"));

    layout->addWidget(icon);
    layout->addWidget(label);
    return item;
}

} // namespace

class RackCanvasWidget : public QWidget
{
public:
    explicit RackCanvasWidget(QWidget *parent = nullptr)
        : QWidget(parent)
        , m_clocks(nullptr)
        , m_slotOwners(nullptr)
        , m_rackIndex(0)
        , m_selectedRow(0)
        , m_selectedCol(0)
        , m_hoverRow(-1)
        , m_hoverCol(-1)
    {
        setMinimumHeight(360);
        setMouseTracking(true);
    }

    void setRackData(const QVector<SubClockInfo> *clocks,
                     const QVector<int> *slotOwners,
                     int rackIndex)
    {
        m_clocks = clocks;
        m_slotOwners = slotOwners;
        m_rackIndex = rackIndex;
        update();
    }

    void setSelection(int row, int col)
    {
        m_selectedRow = row;
        m_selectedCol = col;
        update();
    }

    std::function<void(int, int)> onSlotClicked;
    std::function<void(int, int)> onSlotActivated;
    std::function<void(int, int)> onSlotContextMenuRequested;

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        painter.fillRect(rect(), QColor(QStringLiteral("#09111d")));

        const QRectF panelRect = rect().adjusted(1, 1, -1, -1);
        painter.setPen(QPen(QColor(QStringLiteral("#22324a")), 1));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(panelRect, 18, 18);

        const QRectF gridArea = gridRect();
        const qreal cellWidth = gridArea.width() / kRackColumns;
        const qreal cellHeight = gridArea.height() / kRackRows;

        painter.setPen(QColor(QStringLiteral("#87a0c2")));
        QFont axisFont = painter.font();
        axisFont.setPointSize(10);
        axisFont.setBold(true);
        painter.setFont(axisFont);

        for (int col = 0; col < kRackColumns; ++col) {
            QRectF rectLabel(gridArea.left() + col * cellWidth, gridArea.top() - 24.0, cellWidth, 18.0);
            painter.drawText(rectLabel, Qt::AlignCenter, QString::number(col + 1));
        }

        for (int row = 0; row < kRackRows; ++row) {
            QRectF rectLabel(12.0, gridArea.top() + row * cellHeight, 30.0, cellHeight);
            painter.drawText(rectLabel, Qt::AlignCenter, QString::number(row + 1));
        }

        painter.setPen(QPen(QColor(QStringLiteral("#1f2d43")), 1));
        painter.setBrush(Qt::NoBrush);
        //painter.drawRoundedRect(gridArea.adjusted(-6, -6, 6, 6), 18, 18);

        for (int row = 0; row < kRackRows; ++row) {
            for (int col = 0; col < kRackColumns; ++col) {
                const int ownerIndex = ownerAt(row, col);
                if (ownerIndex >= 0 && isSecondarySlot(ownerIndex, row, col)) {
                    continue;
                }

                if (ownerIndex >= 0 && m_clocks && ownerIndex < m_clocks->size()) {
                    drawClock(painter, m_clocks->at(ownerIndex), row, col);
                } else {
                    drawEmptySlot(painter, row, col);
                }
            }
        }
    }

    void mousePressEvent(QMouseEvent *event) override
    {
        const QPoint cell = cellAt(event->pos());
        if (cell.x() >= 0) {
            if (event->button() == Qt::LeftButton) {
                if (onSlotClicked) {
                    onSlotClicked(cell.y(), cell.x());
                }
                event->accept();
                return;
            }
            if (event->button() == Qt::RightButton) {
                if (onSlotContextMenuRequested) {
                    onSlotContextMenuRequested(cell.y(), cell.x());
                }
                event->accept();
                return;
            }
        }
        QWidget::mousePressEvent(event);
    }

    void mouseDoubleClickEvent(QMouseEvent *event) override
    {
        const QPoint cell = cellAt(event->pos());
        if (cell.x() >= 0 && onSlotActivated) {
            onSlotActivated(cell.y(), cell.x());
        }
        QWidget::mouseDoubleClickEvent(event);
    }

    void mouseMoveEvent(QMouseEvent *event) override
    {
        const QPoint cell = cellAt(event->pos());
        if (cell.x() != m_hoverCol || cell.y() != m_hoverRow) {
            m_hoverCol = cell.x();
            m_hoverRow = cell.y();
            update();
        }

        if (cell.x() >= 0) {
            const int ownerIndex = ownerAt(cell.y(), cell.x());
            if (ownerIndex >= 0 && m_clocks && ownerIndex < m_clocks->size()) {
                const SubClockInfo &clock = m_clocks->at(ownerIndex);
                setToolTip(QString::fromUtf8("测试架 %1\n%2\n类型：%3\n状态：%4\n位置：%5")
                           .arg(clock.rackIndex + 1)
                           .arg(clock.displayName)
                           .arg(shapeText(clock.shape))
                           .arg(stateText(clock.state))
                           .arg(slotPointText(clock.row, clock.column, clock.span)));
                setCursor(Qt::PointingHandCursor);
            } else {
                setToolTip(QString::fromUtf8("测试架 %1\n第%2排 / 第%3挂点\n未安装，点击可设置圆钟或方钟")
                           .arg(m_rackIndex + 1)
                           .arg(cell.y() + 1)
                           .arg(cell.x() + 1));
                setCursor(Qt::PointingHandCursor);
            }
        } else {
            unsetCursor();
            setToolTip(QString());
        }

        QWidget::mouseMoveEvent(event);
    }

    void leaveEvent(QEvent *event) override
    {
        m_hoverRow = -1;
        m_hoverCol = -1;
        unsetCursor();
        update();
        QWidget::leaveEvent(event);
    }

private:
    QRectF gridRect() const
    {
        return rect().adjusted(52, 28, -20, -20);
    }

    QRectF cellRect(int row, int col) const
    {
        const QRectF gridArea = gridRect();
        const qreal cellWidth = gridArea.width() / kRackColumns;
        const qreal cellHeight = gridArea.height() / kRackRows;

        return QRectF(gridArea.left() + col * cellWidth,
                      gridArea.top() + row * cellHeight,
                      cellWidth,
                      cellHeight);
    }

    QRectF slotVisualRect(int row, int col, int span) const
    {
        QRectF rect = cellRect(row, col);
        const qreal cellWidth = rect.width();
        const qreal totalWidth = cellWidth * span;

        return QRectF(rect.left() + 6.0,
                      rect.top() + 10.0,
                      totalWidth - 12.0,
                      rect.height() - 20.0);
    }

    QPoint cellAt(const QPoint &pos) const
    {
        const QRectF gridArea = gridRect();
        if (!gridArea.contains(pos)) {
            return QPoint(-1, -1);
        }

        const qreal cellWidth = gridArea.width() / kRackColumns;
        const qreal cellHeight = gridArea.height() / kRackRows;
        int col = int((pos.x() - gridArea.left()) / cellWidth);
        int row = int((pos.y() - gridArea.top()) / cellHeight);

        if (col < 0 || col >= kRackColumns || row < 0 || row >= kRackRows) {
            return QPoint(-1, -1);
        }
        return QPoint(col, row);
    }

    int ownerAt(int row, int col) const
    {
        if (!m_slotOwners || row < 0 || row >= kRackRows || col < 0 || col >= kRackColumns) {
            return -1;
        }
        return m_slotOwners->value(slotIndex(row, col), -1);
    }

    bool isSecondarySlot(int ownerIndex, int row, int col) const
    {
        if (!m_clocks || ownerIndex < 0 || ownerIndex >= m_clocks->size()) {
            return false;
        }

        const SubClockInfo &clock = m_clocks->at(ownerIndex);
        if (!clock.active || clock.shape != SubClockShape::SquareClock) {
            return false;
        }
        return clock.row == row && clock.column + 1 == col;
    }

    void drawEmptySlot(QPainter &painter, int row, int col)
    {
        const QRectF slotRect = slotVisualRect(row, col, 1);
        const qreal size = qMin(slotRect.width(), slotRect.height()) - 8.0;
        const QRectF circleRect(slotRect.center().x() - size * 0.5,
                                slotRect.center().y() - size * 0.5,
                                size,
                                size);

        const bool selected = (row == m_selectedRow && col == m_selectedCol);
        const bool hovered = (row == m_hoverRow && col == m_hoverCol);

        painter.setBrush(QColor(QStringLiteral("#c7c7cb")));
        painter.setPen(QPen(selected ? QColor(QStringLiteral("#60a5fa"))
                                     : (hovered ? QColor(QStringLiteral("#93c5fd"))
                                                : QColor(QStringLiteral("#d7dee8"))),
                             selected ? 2.6 : (hovered ? 2.0 : 1.2)));
        painter.drawEllipse(circleRect);

        painter.setPen(QPen(QColor(QStringLiteral("#ef4444")), 1.7));
        painter.drawLine(circleRect.topLeft() + QPointF(6.0, 6.0),
                         circleRect.bottomRight() - QPointF(6.0, 6.0));
        painter.drawLine(circleRect.topRight() + QPointF(-6.0, 6.0),
                         circleRect.bottomLeft() + QPointF(6.0, -6.0));
    }

    void drawClock(QPainter &painter, const SubClockInfo &clock, int row, int col)
    {
        const QRectF slotRect = slotVisualRect(row, col, clock.span);
        const bool selected = (clock.row == m_selectedRow && clock.column == m_selectedCol);
        const bool hovered = (ownerAt(m_hoverRow, m_hoverCol) >= 0
                              && ownerAt(m_hoverRow, m_hoverCol) == ownerAt(row, col));

        QColor fill = stateFillColor(clock.state);
        QColor border = stateBorderColor(clock.state);

        QLinearGradient gradient(slotRect.topLeft(), slotRect.bottomRight());
        gradient.setColorAt(0.0, fill.lighter(selected ? 135 : 122));
        gradient.setColorAt(1.0, fill.darker(hovered ? 94 : 108));

        painter.setPen(QPen(selected ? QColor(QStringLiteral("#60a5fa")) : border,
                            selected ? 2.8 : (hovered ? 2.0 : 1.4)));
        painter.setBrush(gradient);

        if (clock.shape == SubClockShape::RoundClock) {
            const qreal size = qMin(slotRect.width(), slotRect.height()) - 6.0;
            QRectF circleRect(slotRect.center().x() - size * 0.5,
                              slotRect.center().y() - size * 0.5,
                              size,
                              size);
            painter.drawEllipse(circleRect);
            drawClockSymbol(painter, circleRect.adjusted(5.0, 5.0, -5.0, -5.0), true, &clock,
                            QColor(QStringLiteral("#f8fafc")), 1.4);
            return;
        }

        painter.drawRoundedRect(slotRect, 12, 12);

        const qreal bubbleSize = qMin(slotRect.height() - 16.0, slotRect.width() * 0.32);
        QRectF bubbleRect(slotRect.left() + 10.0,
                          slotRect.top() + (slotRect.height() - bubbleSize) * 0.5,
                          bubbleSize,
                          bubbleSize);

        painter.setBrush(QColor(255, 255, 255, 26));
        painter.setPen(QPen(QColor(255, 255, 255, 130), 1.1));
        painter.drawEllipse(bubbleRect);
        drawClockSymbol(painter, bubbleRect.adjusted(4.0, 4.0, -4.0, -4.0), true, &clock,
                        QColor(QStringLiteral("#f8fafc")), 1.2);

        painter.setPen(QColor(QStringLiteral("#f8fafc")));
        QFont bubbleFont = painter.font();
        bubbleFont.setPointSize(11);
        bubbleFont.setBold(true);
        painter.setFont(bubbleFont);

        QRectF textRect(bubbleRect.right() + 12.0, slotRect.top(),
                        slotRect.right() - bubbleRect.right() - 18.0, slotRect.height());
        painter.drawText(textRect,
                         Qt::AlignVCenter | Qt::AlignLeft,
                         QString::fromUtf8("方钟"));
    }

    const QVector<SubClockInfo> *m_clocks;
    const QVector<int> *m_slotOwners;
    int m_rackIndex;
    int m_selectedRow;
    int m_selectedCol;
    int m_hoverRow;
    int m_hoverCol;
};

class ClockPreviewWidget : public QWidget
{
public:
    explicit ClockPreviewWidget(QWidget *parent = nullptr)
        : QWidget(parent)
        , m_clock(nullptr)
        , m_hasSelection(false)
        , m_rackIndex(0)
        , m_row(0)
        , m_col(0)
    {
        setMinimumHeight(260);
    }

    void setSelection(const SubClockInfo *clock, int rackIndex, int row, int col)
    {
        m_clock = clock;
        m_hasSelection = true;
        m_rackIndex = rackIndex;
        m_row = row;
        m_col = col;
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.fillRect(rect(), QColor(QStringLiteral("#0b1220")));

        const QRectF panelRect = rect().adjusted(1, 1, -1, -1);
        painter.setPen(QPen(QColor(QStringLiteral("#22324a")), 1));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(panelRect, 18, 18);

        if (!m_hasSelection) {
            painter.setPen(QColor(QStringLiteral("#8fa7c8")));
            painter.drawText(panelRect.adjusted(24, 24, -24, -24),
                             Qt::AlignCenter,
                             QString::fromUtf8("点击左侧挂点开始设置\n双击已安装子钟可放大查看"));
            return;
        }

        const SubClockState state = m_clock ? m_clock->state : SubClockState::Uninstalled;
        const QColor fill = stateFillColor(state);
        const QColor border = stateBorderColor(state);

        QRectF badgeRect(panelRect.left() + 16.0, panelRect.top() + 14.0, 112.0, 28.0);
        painter.setBrush(fill.darker(150));
        painter.setPen(QPen(border, 1.0));
        painter.drawRoundedRect(badgeRect, 14, 14);

        painter.setPen(QColor(QStringLiteral("#ffffff")));
        QFont badgeFont = painter.font();
        badgeFont.setPointSize(10);
        badgeFont.setBold(true);
        painter.setFont(badgeFont);
        painter.drawText(badgeRect, Qt::AlignCenter, stateText(state));

        QRectF shapeRect = panelRect.adjusted(48, 56, -48, -84);
        if (!m_clock || m_clock->shape == SubClockShape::RoundClock) {
            const qreal size = qMin(shapeRect.width(), shapeRect.height()) - 12.0;
            shapeRect = QRectF(shapeRect.center().x() - size * 0.5,
                               shapeRect.center().y() - size * 0.5,
                               size,
                               size);
        } else {
            const qreal targetWidth = qMin(shapeRect.width(), shapeRect.height() * 2.52);
            const qreal targetHeight = targetWidth / 2.52;
            shapeRect = QRectF(shapeRect.center().x() - targetWidth * 0.5,
                               shapeRect.center().y() - targetHeight * 0.5,
                               targetWidth,
                               targetHeight);
        }

        if (!m_clock) {
            painter.setBrush(QColor(QStringLiteral("#c7c7cb")));
            painter.setPen(QPen(border, 2.0));
            painter.drawEllipse(shapeRect);

            painter.setPen(QPen(QColor(QStringLiteral("#ef4444")), 2.1));
            painter.drawLine(shapeRect.topLeft() + QPointF(20.0, 20.0),
                             shapeRect.bottomRight() - QPointF(20.0, 20.0));
            painter.drawLine(shapeRect.topRight() + QPointF(-20.0, 20.0),
                             shapeRect.bottomLeft() + QPointF(20.0, -20.0));
        } else {
            QLinearGradient gradient(shapeRect.topLeft(), shapeRect.bottomRight());
            gradient.setColorAt(0.0, fill.lighter(132));
            gradient.setColorAt(1.0, fill.darker(110));

            painter.setBrush(gradient);
            painter.setPen(QPen(border, 2.4));

            if (m_clock->shape == SubClockShape::RoundClock) {
                painter.drawEllipse(shapeRect);
                drawClockSymbol(painter, shapeRect.adjusted(18.0, 18.0, -18.0, -18.0), true, m_clock,
                                QColor(QStringLiteral("#ffffff")), 3.0);
            } else {
                painter.drawRoundedRect(shapeRect, 18, 18);

                const qreal bubbleSize = qMin(shapeRect.height() - 32.0, shapeRect.width() * 0.26);
                QRectF bubbleRect(shapeRect.left() + 20.0,
                                  shapeRect.top() + (shapeRect.height() - bubbleSize) * 0.5,
                                  bubbleSize,
                                  bubbleSize);

                painter.setBrush(QColor(255, 255, 255, 30));
                painter.setPen(QPen(QColor(255, 255, 255, 140), 1.2));
                painter.drawEllipse(bubbleRect);
                drawClockSymbol(painter, bubbleRect.adjusted(9.0, 9.0, -9.0, -9.0), true, m_clock,
                                QColor(QStringLiteral("#ffffff")), 2.4);

                painter.setPen(QColor(QStringLiteral("#ffffff")));
                QFont typeFont = painter.font();
                typeFont.setPointSize(22);
                typeFont.setBold(true);
                painter.setFont(typeFont);
                QRectF typeRect(bubbleRect.right() + 18.0,
                                shapeRect.top(),
                                shapeRect.right() - bubbleRect.right() - 26.0,
                                shapeRect.height());
                painter.drawText(typeRect, Qt::AlignVCenter | Qt::AlignLeft, QString::fromUtf8("方钟"));
            }
        }

        const QString pointInfo = m_clock
                ? slotPointText(m_clock->row, m_clock->column, m_clock->span)
                : slotPointText(m_row, m_col, 1);
        const QString stamp = m_clock && m_clock->lastMeasured.isValid()
                ? m_clock->lastMeasured.toString(QStringLiteral("yyyy-MM-dd hh:mm:ss"))
                : QString::fromUtf8("--");

        painter.setPen(QColor(QStringLiteral("#9fb5d4")));
        QFont infoFont = painter.font();
        infoFont.setPointSize(11);
        infoFont.setBold(true);
        painter.setFont(infoFont);
        painter.drawText(QRectF(panelRect.left() + 18.0, panelRect.bottom() - 70.0,
                                panelRect.width() - 36.0, 22.0),
                         Qt::AlignCenter,
                         QString::fromUtf8("所在位置：测试架 %1 · %2")
                         .arg(m_rackIndex + 1)
                         .arg(pointInfo));

        infoFont.setPointSize(10);
        infoFont.setBold(false);
        painter.setFont(infoFont);
        painter.drawText(QRectF(panelRect.left() + 18.0, panelRect.bottom() - 42.0,
                                panelRect.width() - 36.0, 20.0),
                         Qt::AlignCenter,
                         QString());
    }

private:
    const SubClockInfo *m_clock;
    bool m_hasSelection;
    int m_rackIndex;
    int m_row;
    int m_col;
};

SubClockMeasurementPage::SubClockMeasurementPage(QWidget *parent)
    : QWidget(parent)
    , m_running(false)
    , m_syncingSelection(false)
    , m_refreshingTable(false)
    , m_currentRackIndex(0)
    , m_selectedRow(0)
    , m_selectedCol(0)
    , m_snapshotSequence(1000)
    , m_rackMetricValue(nullptr)
    , m_installMetricValue(nullptr)
    , m_resultMetricValue(nullptr)
    , m_selectionMetricValue(nullptr)
    , m_selectedTitleLabel(nullptr)
    , m_selectedMetaLabel(nullptr)
    , m_selectedHintLabel(nullptr)
    , m_tableTitleLabel(nullptr)
    , m_progressBar(nullptr)
    , m_resultTable(nullptr)
    , m_previewWidget(nullptr)
    , m_rackCanvas(nullptr)
    , m_rackTabBar(nullptr)
{
    setObjectName(QStringLiteral("subclockMeasurementPage"));

    initialiseRackModel();
    buildUi();
    refreshUi();
}

void SubClockMeasurementPage::initialiseRackModel()
{
    m_running = false;
    m_currentRackIndex = 0;
    m_selectedRow = 0;
    m_selectedCol = 0;
    m_snapshotSequence = 1000;
    m_clocks.clear();
    m_slotOwners = QVector<QVector<int> >(kRackCount, QVector<int>(kRackPointCount, -1));
}

void SubClockMeasurementPage::buildUi()
{
    QVBoxLayout *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    QFrame *card = new QFrame(this);
    card->setObjectName(QStringLiteral("subclockCard"));
    rootLayout->addWidget(card);

    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(10);

    QWidget *metricGrid = new QWidget(card);
    QGridLayout *metricLayout = new QGridLayout(metricGrid);
    metricLayout->setContentsMargins(0, 0, 0, 0);
    metricLayout->setHorizontalSpacing(12);
    metricLayout->setVerticalSpacing(12);

    metricLayout->addWidget(createMetricTile(QString::fromUtf8("当前测试架"),
                                             QString::fromUtf8("测试架 1 / 6"),
                                             &m_rackMetricValue,
                                             metricGrid), 0, 0);
    metricLayout->addWidget(createMetricTile(QString::fromUtf8("安装概况"),
                                             QString::fromUtf8("已安装 0 点 / 未安装 270 点"),
                                             &m_installMetricValue,
                                             metricGrid), 0, 1);
    metricLayout->addWidget(createMetricTile(QString::fromUtf8("测试结果"),
                                             QString::fromUtf8("待测 0 · 合格 0 · 不合格 0"),
                                             &m_resultMetricValue,
                                             metricGrid), 0, 2);
    metricLayout->addWidget(createMetricTile(QString::fromUtf8("当前选择"),
                                             QString::fromUtf8("测试架 1 · 第1排 / 第1挂点 · 未安装"),
                                             &m_selectionMetricValue,
                                             metricGrid), 0, 3);

    layout->addWidget(metricGrid);

    m_progressBar = new QProgressBar(card);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(false);
    m_progressBar->setFixedHeight(8);
    layout->addWidget(m_progressBar);

    m_rackTabBar = new QTabBar(card);
    m_rackTabBar->setObjectName(QStringLiteral("rackTabBar"));
    m_rackTabBar->setExpanding(false);
    m_rackTabBar->setUsesScrollButtons(true);
    m_rackTabBar->setDrawBase(false);
    m_rackTabBar->setDocumentMode(true);
    m_rackTabBar->setElideMode(Qt::ElideRight);
    m_rackTabBar->setFixedHeight(66);

    m_rackTabBar->setStyleSheet(QStringLiteral(
        "QTabBar#rackTabBar {"
        "    background: transparent;"
        "}"
        "QTabBar#rackTabBar::tab {"
        "    min-width: 150px;"
        "    min-height: 52px;"
        "    padding: 10px 22px;"
        "    margin-right: 8px;"
        "    background: #14233a;"
        "    color: #dbe8ff;"
        "    border: 1px solid #2c4468;"
        "    border-top-left-radius: 10px;"
        "    border-top-right-radius: 10px;"
        "    font-size: 30px;"
        "    font-weight: 700;"
        "}"
        "QTabBar#rackTabBar::tab:selected {"
        "    background: #1f3f73;"
        "    color: #ffffff;"
        "    border: 1px solid #4c86ff;"
        "}"
        "QTabBar#rackTabBar::tab:hover:!selected {"
        "    background: #18304f;"
        "}"
    ));
    m_rackTabBar->setUsesScrollButtons(true);
    m_rackTabBar->setDrawBase(false);
    m_rackTabBar->setDocumentMode(true);
    m_rackTabBar->setElideMode(Qt::ElideRight);
    for (int rackIndex = 0; rackIndex < kRackCount; ++rackIndex) {
        m_rackTabBar->addTab(QString::fromUtf8("测试架 %1").arg(rackIndex + 1));
    }
    connect(m_rackTabBar, &QTabBar::currentChanged, this, [this](int index) {
        if (!m_syncingSelection) {
            selectRack(index);
        }
    });

    QWidget *legendActionRow = new QWidget(card);
    QHBoxLayout *legendActionLayout = new QHBoxLayout(legendActionRow);
    legendActionLayout->setContentsMargins(0, 0, 0, 0);
    legendActionLayout->setSpacing(14);

    legendActionLayout->addWidget(createLegendItem(QString::fromUtf8("未安装"),
                                                   stateFillColor(SubClockState::Uninstalled),
                                                   stateBorderColor(SubClockState::Uninstalled),
                                                   true,
                                                   legendActionRow));
    legendActionLayout->addWidget(createLegendItem(QString::fromUtf8("待测试"),
                                                   stateFillColor(SubClockState::Pending),
                                                   stateBorderColor(SubClockState::Pending),
                                                   true,
                                                   legendActionRow));
    legendActionLayout->addWidget(createLegendItem(QString::fromUtf8("测试合格"),
                                                   stateFillColor(SubClockState::Passed),
                                                   stateBorderColor(SubClockState::Passed),
                                                   true,
                                                   legendActionRow));
    legendActionLayout->addWidget(createLegendItem(QString::fromUtf8("测试不合格"),
                                                   stateFillColor(SubClockState::Failed),
                                                   stateBorderColor(SubClockState::Failed),
                                                   true,
                                                   legendActionRow));
    legendActionLayout->addWidget(createLegendItem(QString::fromUtf8("圆钟"),
                                                   QColor(QStringLiteral("#101a2a")),
                                                   QColor(QStringLiteral("#7c90b1")),
                                                   true,
                                                   legendActionRow));
    legendActionLayout->addWidget(createLegendItem(QString::fromUtf8("方钟"),
                                                   QColor(QStringLiteral("#101a2a")),
                                                   QColor(QStringLiteral("#7c90b1")),
                                                   false,
                                                   legendActionRow));
    legendActionLayout->addStretch();

    QPushButton *startButton = createActionButton(QString::fromUtf8("开始 / 复测当前架"), true, legendActionRow,
                                                   QStringLiteral("subclockStartButton"));
    QPushButton *resetButton = createActionButton(QString::fromUtf8("当前架复位为待测试"), false, legendActionRow,
                                                   QStringLiteral("subclockResetButton"));
    QPushButton *clearButton = createActionButton(QString::fromUtf8("清空当前架"), false, legendActionRow,
                                                   QStringLiteral("subclockClearButton"));
    QPushButton *exportButton = createActionButton(QString::fromUtf8("导出结果"), false, legendActionRow,
                                                    QStringLiteral("subclockExportButton"));

    connect(startButton, &QPushButton::clicked, this, &SubClockMeasurementPage::startPreview);
    connect(resetButton, &QPushButton::clicked, this, &SubClockMeasurementPage::resetCurrentRackToPending);
    connect(clearButton, &QPushButton::clicked, this, &SubClockMeasurementPage::clearCurrentRack);
    connect(exportButton, &QPushButton::clicked, this, &SubClockMeasurementPage::exportResult);

    legendActionLayout->addWidget(startButton);
    legendActionLayout->addWidget(resetButton);
    legendActionLayout->addWidget(clearButton);
    legendActionLayout->addWidget(exportButton);

    layout->addWidget(legendActionRow);
    layout->addWidget(m_rackTabBar);

    QWidget *contentRow = new QWidget(card);
    QHBoxLayout *contentLayout = new QHBoxLayout(contentRow);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(16);

    QFrame *layoutFrame = new QFrame(contentRow);
    layoutFrame->setObjectName(QStringLiteral("sectionFrame"));
    QVBoxLayout *layoutFrameLayout = new QVBoxLayout(layoutFrame);
    layoutFrameLayout->setContentsMargins(0, 0, 0, 0);
    layoutFrameLayout->setSpacing(2);

    m_rackCanvas = new RackCanvasWidget(layoutFrame);
    m_rackCanvas->onSlotClicked = [this](int row, int col) {
        selectSlot(m_currentRackIndex, row, col, true);
    };
    m_rackCanvas->onSlotActivated = [this](int row, int col) {
        selectSlot(m_currentRackIndex, row, col, false);
        if (selectedClockIndex() >= 0) {
            showSelectedClockZoom();
        }
    };
    m_rackCanvas->onSlotContextMenuRequested = [this](int row, int col) {
        showSlotContextMenu(m_currentRackIndex, row, col);
    };

    layoutFrameLayout->addWidget(m_rackCanvas, 1);

    QFrame *detailFrame = new QFrame(contentRow);
    detailFrame->setObjectName(QStringLiteral("sectionFrame"));
    detailFrame->setMinimumWidth(380);
    QVBoxLayout *detailLayout = new QVBoxLayout(detailFrame);
    detailLayout->setContentsMargins(18, 18, 18, 20);
    detailLayout->setSpacing(12);

    m_selectedTitleLabel = new QLabel(detailFrame);
    m_selectedTitleLabel->setObjectName(QStringLiteral("sectionTitle"));
    m_selectedTitleLabel->hide();

    m_previewWidget = new ClockPreviewWidget(detailFrame);

    QWidget *detailInfoPanel = new QWidget(detailFrame);
    detailInfoPanel->setObjectName(QStringLiteral("detailInfoPanel"));

    QGridLayout *infoLayout = new QGridLayout(detailInfoPanel);
    infoLayout->setContentsMargins(0, 0, 0, 0);
    infoLayout->setHorizontalSpacing(10);
    infoLayout->setVerticalSpacing(10);

    auto addInfoRow = [&](int row, const QString &title, const QString &valueObjectName) {
        QLabel *keyLabel = new QLabel(title, detailInfoPanel);
        keyLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
        keyLabel->setMinimumWidth(78);

        QFont keyFont = keyLabel->font();
        keyFont.setPointSize(12);
        keyFont.setBold(true);
        keyLabel->setFont(keyFont);
        keyLabel->setStyleSheet(QStringLiteral("color:#9fb5d4; background:transparent;"));

        QLabel *valueLabel = new QLabel(QString::fromUtf8("--"), detailInfoPanel);
        valueLabel->setObjectName(valueObjectName);
        valueLabel->setWordWrap(true);
        valueLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);

        QFont valueFont = valueLabel->font();
        valueFont.setPointSize(12);
        valueFont.setBold(true);
        valueLabel->setFont(valueFont);
        valueLabel->setStyleSheet(QStringLiteral("color:#ffffff; background:transparent;"));

        infoLayout->addWidget(keyLabel, row, 0, Qt::AlignTop);
        infoLayout->addWidget(valueLabel, row, 1, Qt::AlignTop);
    };

    addInfoRow(0, QString::fromUtf8("位置:"), QStringLiteral("detailValuePosition"));
    addInfoRow(1, QString::fromUtf8("类型:"), QStringLiteral("detailValueType"));
    addInfoRow(2, QString::fromUtf8("状态:"), QStringLiteral("detailValueState"));
    addInfoRow(3, QString::fromUtf8("当前时差:"), QStringLiteral("detailValueOffset"));
    addInfoRow(4, QString::fromUtf8("最近时间戳:"), QStringLiteral("detailValueTimestamp"));
    addInfoRow(5, QString::fromUtf8("说明:"), QStringLiteral("detailValueRemark"));

    infoLayout->setColumnStretch(0, 0);
    infoLayout->setColumnStretch(1, 1);
    detailInfoPanel->setMinimumHeight(208);

    QPushButton *zoomButton = createActionButton(QString::fromUtf8("放大查看"), false, detailFrame,
                                                 QStringLiteral("zoomButton"));
    connect(zoomButton, &QPushButton::clicked, this, &SubClockMeasurementPage::showSelectedClockZoom);
    zoomButton->setProperty("actionRole", QStringLiteral("zoom"));

    detailLayout->addWidget(m_selectedTitleLabel);
    detailLayout->addWidget(m_previewWidget, 3);
    detailLayout->addWidget(detailInfoPanel, 2);
    detailLayout->addWidget(zoomButton);
    detailLayout->addStretch(1);

    contentLayout->addWidget(layoutFrame, 3);
    contentLayout->addWidget(detailFrame, 2);

    layout->addWidget(contentRow, 1);

    QFrame *tableFrame = new QFrame(card);
    tableFrame->setObjectName(QStringLiteral("sectionFrame"));
    QVBoxLayout *tableLayout = new QVBoxLayout(tableFrame);
    tableLayout->setContentsMargins(16, 16, 16, 16);
    tableLayout->setSpacing(10);

    m_tableTitleLabel = new QLabel(QString::fromUtf8("测试架 01 子钟明细"), tableFrame);
    m_tableTitleLabel->setObjectName(QStringLiteral("sectionTitle"));

    m_resultTable = new QTableWidget(0, 6, tableFrame);
    m_resultTable->setObjectName(QStringLiteral("pageTable"));
    m_resultTable->setHorizontalHeaderLabels(QStringList()
                                             << QString::fromUtf8("子钟")
                                             << QString::fromUtf8("类型")
                                             << QString::fromUtf8("挂点位置")
                                             << QString::fromUtf8("状态")
                                             << QString::fromUtf8("当前时差(ms)")
                                             << QString::fromUtf8("最近时间戳"));
    m_resultTable->verticalHeader()->setVisible(false);
    m_resultTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_resultTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_resultTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_resultTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_resultTable->setAlternatingRowColors(false);
    m_resultTable->setMinimumHeight(320);

    connect(m_resultTable, &QTableWidget::itemSelectionChanged,
            this, &SubClockMeasurementPage::handleTableSelectionChanged);
    connect(m_resultTable, &QTableWidget::cellDoubleClicked, this,
            [this](int, int) { showSelectedClockZoom(); });

    tableLayout->addWidget(m_tableTitleLabel);
    tableLayout->addWidget(m_resultTable);
    layout->addWidget(tableFrame, 2);
    layout->addWidget(tableFrame);

    selectRack(0);
    selectSlot(0, 0, 0, false);
}

void SubClockMeasurementPage::refreshUi()
{
    refreshMetrics();
    refreshRackButtons();
    refreshRackCanvas();
    refreshSelectionPanel();
    refreshTable();
}

void SubClockMeasurementPage::refreshMetrics()
{
    int occupiedPoints = 0;
    for (int rackIndex = 0; rackIndex < kRackCount; ++rackIndex) {
        occupiedPoints += occupiedPointCount(rackIndex);
    }

    m_rackMetricValue->setText(QString::fromUtf8("测试架 %1 / %2")
                               .arg(m_currentRackIndex + 1)
                               .arg(kRackCount));
    m_installMetricValue->setText(QString::fromUtf8("已安装 %1 点 / 未安装 %2 点")
                                  .arg(occupiedPoints)
                                  .arg(kRackCount * kRackPointCount - occupiedPoints));
    m_resultMetricValue->setText(QString::fromUtf8("待测 %1 · 合格 %2 · 不合格 %3")
                                 .arg(totalPendingCount())
                                 .arg(totalPassedCount())
                                 .arg(totalFailedCount()));
    m_selectionMetricValue->setText(currentSelectionText());

    const int installed = totalInstalledCount();
    const int tested = totalPassedCount() + totalFailedCount();
    m_progressBar->setValue(installed > 0 ? (tested * 100) / installed : 0);
}

void SubClockMeasurementPage::refreshRackButtons()
{
    if (!m_rackTabBar) {
        return;
    }

    m_syncingSelection = true;
    for (int rackIndex = 0; rackIndex < kRackCount; ++rackIndex) {
        m_rackTabBar->setTabText(rackIndex,
                                 QString::fromUtf8("测试架 %1")
                                 .arg(rackIndex + 1));
        m_rackTabBar->setTabToolTip(rackIndex, rackSummaryText(rackIndex));
    }
    m_rackTabBar->setCurrentIndex(m_currentRackIndex);
    m_syncingSelection = false;
}

void SubClockMeasurementPage::refreshRackCanvas()
{
    if (!m_rackCanvas) {
        return;
    }

    m_rackCanvas->setRackData(&m_clocks,
                              &m_slotOwners[m_currentRackIndex],
                              m_currentRackIndex);
    m_rackCanvas->setSelection(m_selectedRow, m_selectedCol);
}

void SubClockMeasurementPage::refreshSelectionPanel()
{
    const int clockIndex = selectedClockIndex();
    const SubClockInfo *clock = (clockIndex >= 0 && clockIndex < m_clocks.size() && m_clocks.at(clockIndex).active)
            ? &m_clocks.at(clockIndex)
            : nullptr;

    const QString slotText = slotPointText(m_selectedRow, m_selectedCol, clock ? clock->span : 1);

    auto setDetailValue = [this](const QString &objectName, const QString &text) {
        QLabel *label = findChild<QLabel *>(objectName);
        if (label) {
            label->setText(text);
        }
    };

    if (clock) {
        m_selectedTitleLabel->setText(QString::fromUtf8("测试架 %1 · %2")
                                      .arg(clock->rackIndex + 1)
                                      .arg(clock->displayName));

        setDetailValue(QStringLiteral("detailValuePosition"),
                       slotPointText(clock->row, clock->column, clock->span));
        setDetailValue(QStringLiteral("detailValueType"),
                       shapeText(clock->shape));
        setDetailValue(QStringLiteral("detailValueState"),
                       stateText(clock->state));
        setDetailValue(QStringLiteral("detailValueOffset"),
                       QString::fromUtf8("%1 ms").arg(offsetText(*clock)));
        setDetailValue(QStringLiteral("detailValueTimestamp"),
                       timeText(*clock));
        setDetailValue(QStringLiteral("detailValueRemark"),
                       clock->remark.isEmpty() ? QString::fromUtf8("等待测试") : clock->remark);

        m_previewWidget->setSelection(clock, clock->rackIndex, clock->row, clock->column);
    } else {
        m_selectedTitleLabel->setText(QString::fromUtf8("测试架 %1 · %2")
                                      .arg(m_currentRackIndex + 1)
                                      .arg(slotText));

        setDetailValue(QStringLiteral("detailValuePosition"), slotText);
        setDetailValue(QStringLiteral("detailValueType"), QString::fromUtf8("未安装"));
        setDetailValue(QStringLiteral("detailValueState"), QString::fromUtf8("未安装"));
        setDetailValue(QStringLiteral("detailValueOffset"), QString::fromUtf8("--"));
        setDetailValue(QStringLiteral("detailValueTimestamp"), QString::fromUtf8("--"));
        setDetailValue(QStringLiteral("detailValueRemark"),
                       QString::fromUtf8("点击左侧空挂点可设置圆钟或方钟"));

        m_previewWidget->setSelection(nullptr, m_currentRackIndex, m_selectedRow, m_selectedCol);
    }

    QPushButton *zoomButton = findChild<QPushButton *>(QStringLiteral("zoomButton"));
    if (zoomButton) {
        zoomButton->setEnabled(clock != nullptr);
    }
}

void SubClockMeasurementPage::refreshTable()
{
    if (!m_resultTable) {
        return;
    }

    m_refreshingTable = true;
    QSignalBlocker blocker(m_resultTable);

    const QVector<int> indexes = clockIndexesForRack(m_currentRackIndex);

    m_resultTable->clearContents();
    m_resultTable->setRowCount(indexes.size());

    int selectedRow = -1;
    for (int row = 0; row < indexes.size(); ++row) {
        const int clockIndex = indexes.at(row);
        if (clockIndex < 0 || clockIndex >= m_clocks.size()) {
            continue;
        }

        const SubClockInfo &clock = m_clocks.at(clockIndex);
        if (!clock.active) {
            continue;
        }

        if (clock.row == m_selectedRow && clock.column == m_selectedCol) {
            selectedRow = row;
        }

        QTableWidgetItem *nameItem = new QTableWidgetItem(clock.displayName);
        nameItem->setData(Qt::UserRole, clockIndex);

        QTableWidgetItem *shapeItem = new QTableWidgetItem(shapeText(clock.shape));
        QTableWidgetItem *pointItem = new QTableWidgetItem(slotPointText(clock.row, clock.column, clock.span));
        QTableWidgetItem *stateItem = new QTableWidgetItem(stateText(clock.state));
        QTableWidgetItem *offsetItem = new QTableWidgetItem(offsetText(clock));
        QTableWidgetItem *timeItem = new QTableWidgetItem(timeText(clock));

        QList<QTableWidgetItem *> items;
        items << nameItem << shapeItem << pointItem << stateItem << offsetItem << timeItem;

        const QColor stateColor = stateFillColor(clock.state);
        for (int col = 0; col < items.size(); ++col) {
            QTableWidgetItem *item = items.at(col);
            item->setTextAlignment(Qt::AlignCenter);
            if (col == 3 || col == 4) {
                item->setForeground(QBrush(stateColor));
            }
            m_resultTable->setItem(row, col, item);
        }
    }

    m_tableTitleLabel->setText(QString::fromUtf8("测试架 %1 子钟明细（已安装 %2 / 待测 %3 / 合格 %4 / 不合格 %5）")
                               .arg(m_currentRackIndex + 1, 2, 10, QLatin1Char('0'))
                               .arg(indexes.size())
                               .arg(rackPendingCount(m_currentRackIndex))
                               .arg(rackPassedCount(m_currentRackIndex))
                               .arg(rackFailedCount(m_currentRackIndex)));

    if (selectedRow >= 0 && selectedRow < m_resultTable->rowCount()) {
        m_resultTable->selectRow(selectedRow);
    } else {
        m_resultTable->clearSelection();
    }

    m_refreshingTable = false;
}

void SubClockMeasurementPage::selectRack(int rackIndex)
{
    if (rackIndex < 0 || rackIndex >= kRackCount) {
        return;
    }

    m_currentRackIndex = rackIndex;

    int owner = clockIndexAt(m_currentRackIndex, m_selectedRow, m_selectedCol);
    if (owner < 0) {
        const QVector<int> indexes = clockIndexesForRack(m_currentRackIndex);
        if (!indexes.isEmpty()) {
            const SubClockInfo &clock = m_clocks.at(indexes.first());
            m_selectedRow = clock.row;
            m_selectedCol = clock.column;
        } else {
            m_selectedRow = 0;
            m_selectedCol = 0;
        }
    } else {
        const SubClockInfo &clock = m_clocks.at(owner);
        m_selectedRow = clock.row;
        m_selectedCol = clock.column;
    }

    refreshUi();
}

void SubClockMeasurementPage::selectSlot(int rackIndex, int row, int col, bool openDialogIfEmpty)
{
    if (rackIndex < 0 || rackIndex >= kRackCount || row < 0 || row >= kRackRows || col < 0 || col >= kRackColumns) {
        return;
    }

    m_currentRackIndex = rackIndex;
    const int owner = clockIndexAt(rackIndex, row, col);
    if (owner >= 0 && owner < m_clocks.size() && m_clocks.at(owner).active) {
        const SubClockInfo &clock = m_clocks.at(owner);
        m_selectedRow = clock.row;
        m_selectedCol = clock.column;
        refreshUi();
        return;
    }

    m_selectedRow = row;
    m_selectedCol = col;
    refreshUi();

    if (openDialogIfEmpty) {
        openInstallDialogForSlot(rackIndex, row, col);
    }
}

int SubClockMeasurementPage::selectedClockIndex() const
{
    return clockIndexAt(m_currentRackIndex, m_selectedRow, m_selectedCol);
}

int SubClockMeasurementPage::clockIndexAt(int rackIndex, int row, int col) const
{
    if (rackIndex < 0 || rackIndex >= m_slotOwners.size()
            || row < 0 || row >= kRackRows
            || col < 0 || col >= kRackColumns) {
        return -1;
    }

    const int index = m_slotOwners.at(rackIndex).value(slotIndex(row, col), -1);
    if (index < 0 || index >= m_clocks.size()) {
        return -1;
    }
    return m_clocks.at(index).active ? index : -1;
}

QVector<int> SubClockMeasurementPage::clockIndexesForRack(int rackIndex) const
{
    QVector<int> indexes;
    for (int index = 0; index < m_clocks.size(); ++index) {
        const SubClockInfo &clock = m_clocks.at(index);
        if (clock.active && clock.rackIndex == rackIndex) {
            indexes.append(index);
        }
    }

    std::sort(indexes.begin(), indexes.end(), [this](int left, int right) {
        const SubClockInfo &a = m_clocks.at(left);
        const SubClockInfo &b = m_clocks.at(right);
        if (a.row != b.row) {
            return a.row < b.row;
        }
        return a.column < b.column;
    });
    return indexes;
}

void SubClockMeasurementPage::showSlotContextMenu(int rackIndex, int row, int col)
{
    selectSlot(rackIndex, row, col, false);

    const int clockIndex = clockIndexAt(rackIndex, row, col);
    if (clockIndex < 0 || clockIndex >= m_clocks.size() || !m_clocks.at(clockIndex).active) {
        return;
    }

    QMenu menu(this);
    menu.setObjectName(QStringLiteral("subclockContextMenu"));

    QAction *removeAction = menu.addAction(QString::fromUtf8("删除当前子钟"));
    QAction *selectedAction = menu.exec(QCursor::pos());
    if (selectedAction == removeAction) {
        const int currentIndex = selectedClockIndex();
        if (currentIndex >= 0 && currentIndex < m_clocks.size() && m_clocks.at(currentIndex).active) {
            const QString name = m_clocks.at(currentIndex).displayName;
            removeClock(currentIndex);
            emit videoStateChanged(totalInstalledCount() > 0);
            emit logMessage(QString::fromUtf8("子钟时差测量：%1 已通过右键菜单删除，挂点恢复为未安装").arg(name));
        }
    }
}

void SubClockMeasurementPage::openInstallDialogForSlot(int rackIndex, int row, int col)
{
    if (clockIndexAt(rackIndex, row, col) >= 0) {
        return;
    }

    while (true) {
        QMessageBox box(this);
        box.setWindowTitle(QString::fromUtf8("设置挂点类型"));
        box.setIcon(QMessageBox::Question);
        box.setText(QString::fromUtf8("测试架 %1 · %2")
                    .arg(rackIndex + 1)
                    .arg(slotPointText(row, col, 1)));
        box.setInformativeText(QString::fromUtf8("请选择该挂点安装的子钟类型。"));

        QPushButton *roundButton = box.addButton(QString::fromUtf8("安装圆钟"), QMessageBox::AcceptRole);
        QPushButton *squareButton = box.addButton(QString::fromUtf8("安装方钟"), QMessageBox::ActionRole);
        QPushButton *cancelButton = box.addButton(QMessageBox::Cancel);

        Q_UNUSED(cancelButton);

        box.exec();

        if (box.clickedButton() == roundButton) {
            installClock(rackIndex, row, col, SubClockShape::RoundClock);
            return;
        }

        if (box.clickedButton() == squareButton) {
            if (canPlaceSquareClock(rackIndex, row, col)) {
                installClock(rackIndex, row, col, SubClockShape::SquareClock);
                return;
            }
            showSquareClockInstallUnavailableMessage(rackIndex, row, col);
            continue;
        }

        return;
    }
}

void SubClockMeasurementPage::showSquareClockInstallUnavailableMessage(int rackIndex, int row, int col)
{
    QMessageBox::information(this,
                             QString::fromUtf8("无法安装方钟"),
                             QString::fromUtf8("测试架 %1 · %2 不能安装方钟。\n\n原因：右侧没有可用相邻挂点，或相邻挂点已被其它子钟占用。\n请改为安装圆钟，或先释放右侧相邻挂点后再试。")
                             .arg(rackIndex + 1)
                             .arg(slotPointText(row, col, 1)));
}

bool SubClockMeasurementPage::canPlaceSquareClock(int rackIndex, int row, int col, int ignoreClockIndex) const
{
    if (col >= kRackColumns - 1) {
        return false;
    }

    const int leftOwner = clockIndexAt(rackIndex, row, col);
    const int rightOwner = clockIndexAt(rackIndex, row, col + 1);

    const bool leftOk = (leftOwner < 0 || leftOwner == ignoreClockIndex);
    const bool rightOk = (rightOwner < 0 || rightOwner == ignoreClockIndex);
    return leftOk && rightOk;
}

void SubClockMeasurementPage::installClock(int rackIndex, int row, int col, SubClockShape shape)
{
    if (clockIndexAt(rackIndex, row, col) >= 0) {
        return;
    }
    if (shape == SubClockShape::SquareClock && !canPlaceSquareClock(rackIndex, row, col)) {
        QMessageBox::warning(this,
                             QString::fromUtf8("无法设置方钟"),
                             QString::fromUtf8("当前挂点右侧没有可用相邻挂点，不能设置方钟。"));
        return;
    }

    SubClockInfo clock;
    clock.displayName = QString::fromUtf8("子钟-%1")
            .arg(slotIndex(row, col) + 1, 2, 10, QLatin1Char('0'));
    clock.rackIndex = rackIndex;
    clock.indexInRack = slotIndex(row, col) + 1;
    clock.row = row;
    clock.column = col;
    clock.span = (shape == SubClockShape::RoundClock) ? 1 : 2;
    clock.shape = shape;
    clock.state = SubClockState::Pending;
    clock.offsetMs = 0.0;
    clock.resultText = QString::fromUtf8("等待测试");
    clock.evidencePath.clear();
    clock.databaseId = QString::fromUtf8("SUB-%1-%2")
            .arg(rackIndex + 1, 2, 10, QLatin1Char('0'))
            .arg(clock.indexInRack, 2, 10, QLatin1Char('0'));
    clock.remark = QString::fromUtf8("已安装，等待当前测试架开始测试");
    clock.lastMeasured = QDateTime();
    clock.active = true;

    const int clockIndex = m_clocks.size();
    m_clocks.append(clock);

    m_slotOwners[rackIndex][slotIndex(row, col)] = clockIndex;
    if (clock.span == 2) {
        m_slotOwners[rackIndex][slotIndex(row, col + 1)] = clockIndex;
    }

    m_selectedRow = row;
    m_selectedCol = col;
    refreshUi();
    emit videoStateChanged(true);
    emit logMessage(QString::fromUtf8("子钟时差测量：测试架 %1 的 %2 已设置为%3")
                    .arg(rackIndex + 1)
                    .arg(slotPointText(row, col, clock.span))
                    .arg(shape == SubClockShape::RoundClock
                             ? QString::fromUtf8("圆钟")
                             : QString::fromUtf8("方钟")));
}

void SubClockMeasurementPage::convertClockShape(int clockIndex, SubClockShape shape)
{
    if (clockIndex < 0 || clockIndex >= m_clocks.size() || !m_clocks.at(clockIndex).active) {
        return;
    }

    SubClockInfo &clock = m_clocks[clockIndex];
    const int rackIndex = clock.rackIndex;
    const int row = clock.row;
    const int col = clock.column;

    m_slotOwners[rackIndex][slotIndex(row, col)] = -1;
    if (clock.span == 2) {
        m_slotOwners[rackIndex][slotIndex(row, col + 1)] = -1;
    }

    const int newSpan = (shape == SubClockShape::RoundClock) ? 1 : 2;
    if (shape == SubClockShape::SquareClock && !canPlaceSquareClock(rackIndex, row, col, clockIndex)) {
        m_slotOwners[rackIndex][slotIndex(row, col)] = clockIndex;
        if (clock.span == 2) {
            m_slotOwners[rackIndex][slotIndex(row, col + 1)] = clockIndex;
        }
        QMessageBox::warning(this,
                             QString::fromUtf8("无法切换为方钟"),
                             QString::fromUtf8("右侧相邻挂点不可用，当前子钟不能切换为方钟。"));
        return;
    }

    clock.shape = shape;
    clock.span = newSpan;
    clock.state = SubClockState::Pending;
    clock.offsetMs = 0.0;
    clock.resultText = QString::fromUtf8("等待测试");
    clock.evidencePath.clear();
    clock.remark = QString::fromUtf8("已手动调整类型，等待复测");
    clock.lastMeasured = QDateTime();

    m_slotOwners[rackIndex][slotIndex(row, col)] = clockIndex;
    if (clock.span == 2) {
        m_slotOwners[rackIndex][slotIndex(row, col + 1)] = clockIndex;
    }

    m_selectedRow = row;
    m_selectedCol = col;
    refreshUi();
    emit logMessage(QString::fromUtf8("子钟时差测量：%1 已切换为%2")
                    .arg(clock.displayName)
                    .arg(shape == SubClockShape::RoundClock
                             ? QString::fromUtf8("圆钟")
                             : QString::fromUtf8("方钟")));
}

void SubClockMeasurementPage::removeClock(int clockIndex)
{
    if (clockIndex < 0 || clockIndex >= m_clocks.size() || !m_clocks.at(clockIndex).active) {
        return;
    }

    SubClockInfo &clock = m_clocks[clockIndex];

    const int rackIndex = clock.rackIndex;
    const int row = clock.row;
    const int col = clock.column;
    const int span = clock.span;

    m_slotOwners[rackIndex][slotIndex(row, col)] = -1;
    if (span == 2 && col + 1 < kRackColumns) {
        m_slotOwners[rackIndex][slotIndex(row, col + 1)] = -1;
    }

    clock.active = false;
    clock.state = SubClockState::Uninstalled;
    clock.offsetMs = 0.0;
    clock.resultText = QString::fromUtf8("已移除");
    clock.evidencePath.clear();
    clock.remark = QString::fromUtf8("挂点已清空，等待重新安装");
    clock.lastMeasured = QDateTime();

    const QVector<int> remain = clockIndexesForRack(rackIndex);
    if (!remain.isEmpty()) {
        int newSelectedIndex = remain.first();
        for (int idx : remain) {
            const SubClockInfo &c = m_clocks.at(idx);
            if (c.row > row || (c.row == row && c.column >= col)) {
                newSelectedIndex = idx;
                break;
            }
        }

        const SubClockInfo &nextClock = m_clocks.at(newSelectedIndex);
        m_selectedRow = nextClock.row;
        m_selectedCol = nextClock.column;
    } else {
        m_selectedRow = row;
        m_selectedCol = col;
    }

    refreshUi();
}

void SubClockMeasurementPage::startPreview()
{
    const QVector<int> indexes = clockIndexesForRack(m_currentRackIndex);
    if (indexes.isEmpty()) {
        QMessageBox::information(this,
                                 QString::fromUtf8("当前架为空"),
                                 QString::fromUtf8("请先在测试架 %1 上安装圆钟或方钟，再开始测试。")
                                 .arg(m_currentRackIndex + 1));
        emit logMessage(QString::fromUtf8("子钟时差测量：测试架 %1 尚未安装任何子钟")
                        .arg(m_currentRackIndex + 1));
        return;
    }

    testRack(m_currentRackIndex);
    refreshUi();
}

void SubClockMeasurementPage::resetCurrentRackToPending()
{
    const QVector<int> indexes = clockIndexesForRack(m_currentRackIndex);
    if (indexes.isEmpty()) {
        return;
    }

    for (int clockIndex : indexes) {
        SubClockInfo &clock = m_clocks[clockIndex];
        clock.state = SubClockState::Pending;
        clock.offsetMs = 0.0;
        clock.resultText = QString::fromUtf8("等待测试");
        clock.evidencePath.clear();
        clock.remark = QString::fromUtf8("已复位为待测试状态");
        clock.lastMeasured = QDateTime();
    }

    refreshUi();
    emit logMessage(QString::fromUtf8("子钟时差测量：测试架 %1 已全部复位为待测试")
                    .arg(m_currentRackIndex + 1));
}

void SubClockMeasurementPage::clearCurrentRack()
{
    const QVector<int> indexes = clockIndexesForRack(m_currentRackIndex);
    if (indexes.isEmpty()) {
        return;
    }

    for (int slot = 0; slot < m_slotOwners[m_currentRackIndex].size(); ++slot) {
        m_slotOwners[m_currentRackIndex][slot] = -1;
    }

    for (int clockIndex : indexes) {
        SubClockInfo &clock = m_clocks[clockIndex];
        clock.active = false;
        clock.state = SubClockState::Uninstalled;
        clock.offsetMs = 0.0;
        clock.resultText = QString::fromUtf8("已移除");
        clock.evidencePath.clear();
        clock.remark = QString::fromUtf8("挂点已清空，等待重新安装");
        clock.lastMeasured = QDateTime();
    }

    m_selectedRow = 0;
    m_selectedCol = 0;
    m_running = totalInstalledCount() > 0;
    refreshUi();
    emit videoStateChanged(m_running);
    emit logMessage(QString::fromUtf8("子钟时差测量：测试架 %1 已清空，全部挂点恢复未安装状态")
                    .arg(m_currentRackIndex + 1));
}

void SubClockMeasurementPage::exportResult()
{
    QString suggestedName = QString::fromUtf8("subclock_measurement_%1.csv")
            .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_hhmmss")));
    const QString filePath = QFileDialog::getSaveFileName(
                this,
                QString::fromUtf8("导出子钟测试结果"),
                suggestedName,
                QString::fromUtf8("CSV 文件 (*.csv);;文本文件 (*.txt)"));
    if (filePath.isEmpty()) {
        return;
    }

    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this,
                             QString::fromUtf8("导出失败"),
                             QString::fromUtf8("无法写入文件：%1").arg(filePath));
        return;
    }

    QTextStream stream(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    stream.setCodec("UTF-8");
#endif
    stream << QString::fromUtf8("测试架,子钟,类型,挂点位置,状态,当前时差(ms),最近时间戳,结果说明,证据路径\n");

    for (int rackIndex = 0; rackIndex < kRackCount; ++rackIndex) {
        const QVector<int> indexes = clockIndexesForRack(rackIndex);
        for (int clockIndex : indexes) {
            const SubClockInfo &clock = m_clocks.at(clockIndex);
            QStringList row;
            row << QString::number(rackIndex + 1)
                << clock.displayName
                << shapeText(clock.shape)
                << slotPointText(clock.row, clock.column, clock.span)
                << stateText(clock.state)
                << offsetText(clock)
                << timeText(clock)
                << (clock.remark.isEmpty() ? QString::fromUtf8("--") : clock.remark)
                << (clock.evidencePath.isEmpty() ? QString::fromUtf8("--") : clock.evidencePath);
            for (int i = 0; i < row.size(); ++i) {
                row[i].replace('"', QStringLiteral("\"\""));
                row[i] = QStringLiteral("\"") + row[i] + QStringLiteral("\"");
            }
            stream << row.join(QStringLiteral(",")) << "\n";
        }
    }

    if (!file.commit()) {
        QMessageBox::warning(this,
                             QString::fromUtf8("导出失败"),
                             QString::fromUtf8("保存文件失败：%1").arg(filePath));
        return;
    }

    QMessageBox::information(this,
                             QString::fromUtf8("导出成功"),
                             QString::fromUtf8("测试结果已导出到：\n%1").arg(filePath));
    emit logMessage(QString::fromUtf8("子钟时差测量：结果已导出到 %1（待测 %2，合格 %3，不合格 %4）")
                    .arg(filePath)
                    .arg(totalPendingCount())
                    .arg(totalPassedCount())
                    .arg(totalFailedCount()));
}

void SubClockMeasurementPage::handleTableSelectionChanged()
{
    if (m_syncingSelection || m_refreshingTable || !m_resultTable) {
        return;
    }

    const int row = m_resultTable->currentRow();
    if (row < 0 || row >= m_resultTable->rowCount()) {
        return;
    }

    QTableWidgetItem *item = m_resultTable->item(row, 0);
    if (!item) {
        return;
    }

    bool ok = false;
    const int clockIndex = item->data(Qt::UserRole).toInt(&ok);
    if (!ok || clockIndex < 0 || clockIndex >= m_clocks.size()) {
        return;
    }

    const SubClockInfo &clock = m_clocks.at(clockIndex);
    if (!clock.active) {
        return;
    }

    if (clock.rackIndex != m_currentRackIndex) {
        return;
    }

    m_selectedRow = clock.row;
    m_selectedCol = clock.column;

    refreshMetrics();
    refreshRackCanvas();
    refreshSelectionPanel();
}

void SubClockMeasurementPage::showSelectedClockZoom()
{
    const int clockIndex = selectedClockIndex();
    if (clockIndex < 0 || clockIndex >= m_clocks.size() || !m_clocks.at(clockIndex).active) {
        return;
    }

    const SubClockInfo &clock = m_clocks.at(clockIndex);

    QDialog dialog(this);
    dialog.setWindowTitle(QString::fromUtf8("子钟放大查看"));
    dialog.resize(560, 680);
    dialog.setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    dialog.setStyleSheet(QStringLiteral(
        "QDialog { background:#0d1524; }"
        "QLabel { color:#e8f0ff; }"
        "QPushButton {"
        " background:#142238; color:#e7f0ff; border:1px solid #31445f; border-radius:18px;"
        " padding:8px 14px; font-size:13px; font-weight:700; min-height:36px; }"
        "QPushButton:hover { background:#1a2d48; }"));

    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(18, 18, 18, 18);
    layout->setSpacing(12);

    QLabel *title = new QLabel(QString::fromUtf8("测试架 %1 · %2")
                               .arg(clock.rackIndex + 1)
                               .arg(clock.displayName), &dialog);
    title->setStyleSheet(QStringLiteral("font-size:20px;font-weight:800;color:#ffffff;"));


    ClockPreviewWidget *preview = new ClockPreviewWidget(&dialog);
    preview->setMinimumHeight(380);
    preview->setSelection(&clock, clock.rackIndex, clock.row, clock.column);

    QLabel *meta = new QLabel(QString::fromUtf8(
        "类型：%1\n"
        "状态：%2\n"
        "位置：%3\n"
        "当前时差：%4 ms\n"
        "结果说明：%5\n"
        "证据路径：%6")
                              .arg(shapeText(clock.shape))
                              .arg(stateText(clock.state))
                              .arg(slotPointText(clock.row, clock.column, clock.span))
                              .arg(offsetText(clock))
                              .arg(clock.remark.isEmpty() ? QString::fromUtf8("等待测试") : clock.remark)
                              .arg(clock.evidencePath.isEmpty() ? QString::fromUtf8("尚未生成") : clock.evidencePath), &dialog);
    meta->setWordWrap(true);
    meta->setStyleSheet(QStringLiteral("font-size:13px;line-height:1.6em;color:#dbe6f5;"));

    QLabel *timestampLabel = new QLabel(QString::fromUtf8("时间戳：%1")
                                        .arg(timeText(clock)), &dialog);
    timestampLabel->setStyleSheet(QStringLiteral(
        "background:#111d31;"
        "border:1px solid #27405e;"
        "border-radius:12px;"
        "padding:10px 14px;"
        "font-size:13px;"
        "font-weight:700;"
        "color:#ffffff;"));

    QWidget *installRow = new QWidget(&dialog);
    QHBoxLayout *installLayout = new QHBoxLayout(installRow);
    installLayout->setContentsMargins(0, 0, 0, 0);
    installLayout->setSpacing(10);

    QPushButton *roundButton = createActionButton(QString::fromUtf8("设为圆钟"), true, installRow);
    QPushButton *squareButton = createActionButton(QString::fromUtf8("设为方钟"), false, installRow);
    QPushButton *removeButton = createActionButton(QString::fromUtf8("清除安装"), false, installRow);

    squareButton->setEnabled(canPlaceSquareClock(clock.rackIndex, clock.row, clock.column, clockIndex) || clock.shape == SubClockShape::SquareClock);

    connect(roundButton, &QPushButton::clicked, &dialog, [this, &dialog, clockIndex]() {
        convertClockShape(clockIndex, SubClockShape::RoundClock);
        dialog.accept();
    });
    connect(squareButton, &QPushButton::clicked, &dialog, [this, &dialog, clockIndex]() {
        convertClockShape(clockIndex, SubClockShape::SquareClock);
        dialog.accept();
    });
    connect(removeButton, &QPushButton::clicked, &dialog, [this, &dialog]() {
        removeSelectedClock();
        dialog.accept();
    });

    installLayout->addWidget(roundButton);
    installLayout->addWidget(squareButton);
    installLayout->addWidget(removeButton);

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    layout->addWidget(title);
    layout->addWidget(preview);
    layout->addWidget(meta);
    layout->addWidget(timestampLabel);
    layout->addWidget(installRow);
    layout->addWidget(buttons);

    dialog.exec();
}

void SubClockMeasurementPage::setSelectedAsRoundClock()
{
}

void SubClockMeasurementPage::setSelectedAsSquareClock()
{
}

void SubClockMeasurementPage::removeSelectedClock()
{
    const int clockIndex = selectedClockIndex();
    if (clockIndex < 0) {
        return;
    }

    QString name = m_clocks.at(clockIndex).displayName;
    removeClock(clockIndex);
    emit videoStateChanged(totalInstalledCount() > 0);
    emit logMessage(QString::fromUtf8("子钟时差测量：%1 已从当前挂点移除").arg(name));
}

void SubClockMeasurementPage::testRack(int rackIndex)
{
    const QVector<int> indexes = clockIndexesForRack(rackIndex);
    int passed = 0;
    int failed = 0;

    for (int clockIndex : indexes) {
        SubClockInfo &clock = m_clocks[clockIndex];
        measureClock(clock);
        if (clock.state == SubClockState::Passed) {
            ++passed;
        } else if (clock.state == SubClockState::Failed) {
            ++failed;
        }
    }

    m_running = !indexes.isEmpty();
    emit videoStateChanged(m_running);
    emit logMessage(QString::fromUtf8("子钟时差测量：测试架 %1 完成测试 / 复测，合格 %2，不合格 %3")
                    .arg(rackIndex + 1)
                    .arg(passed)
                    .arg(failed));
}

void SubClockMeasurementPage::measureClock(SubClockInfo &clock)
{
    const int failThreshold = (clock.shape == SubClockShape::SquareClock) ? 20 : 14;
    const bool failed = QRandomGenerator::global()->bounded(100) < failThreshold;

    if (failed) {
        const double sign = QRandomGenerator::global()->bounded(2) == 0 ? -1.0 : 1.0;
        clock.offsetMs = sign * ((60.0 + QRandomGenerator::global()->bounded(120)) / 1000.0);
        clock.state = SubClockState::Failed;
        clock.resultText = QString::fromUtf8("时差超阈值");
        clock.remark = QString::fromUtf8("建议对该子钟进行复测或重新校准挂载位置");
    } else {
        clock.offsetMs = (QRandomGenerator::global()->bounded(41) - 20) / 1000.0;
        clock.state = SubClockState::Passed;
        clock.resultText = QString::fromUtf8("测试通过");
        clock.remark = QString::fromUtf8("时差在阈值范围内，可判定为测试合格");
    }

    clock.lastMeasured = QDateTime::currentDateTime();
    clock.evidencePath = QString::fromUtf8("rack_%1/%2/frame_%3.jpg")
            .arg(clock.rackIndex + 1, 2, 10, QLatin1Char('0'))
            .arg(clock.displayName.toLower())
            .arg(m_snapshotSequence++);
}

int SubClockMeasurementPage::totalInstalledCount() const
{
    int count = 0;
    for (const SubClockInfo &clock : m_clocks) {
        if (clock.active) {
            ++count;
        }
    }
    return count;
}

int SubClockMeasurementPage::totalPendingCount() const
{
    int count = 0;
    for (const SubClockInfo &clock : m_clocks) {
        if (clock.active && clock.state == SubClockState::Pending) {
            ++count;
        }
    }
    return count;
}

int SubClockMeasurementPage::totalPassedCount() const
{
    int count = 0;
    for (const SubClockInfo &clock : m_clocks) {
        if (clock.active && clock.state == SubClockState::Passed) {
            ++count;
        }
    }
    return count;
}

int SubClockMeasurementPage::totalFailedCount() const
{
    int count = 0;
    for (const SubClockInfo &clock : m_clocks) {
        if (clock.active && clock.state == SubClockState::Failed) {
            ++count;
        }
    }
    return count;
}

int SubClockMeasurementPage::occupiedPointCount(int rackIndex) const
{
    int count = 0;
    if (rackIndex < 0 || rackIndex >= m_slotOwners.size()) {
        return count;
    }

    const QVector<int> &owners = m_slotOwners.at(rackIndex);
    for (int slotOwner : owners) {
        if (slotOwner >= 0) {
            ++count;
        }
    }
    return count;
}

int SubClockMeasurementPage::rackPendingCount(int rackIndex) const
{
    int count = 0;
    const QVector<int> indexes = clockIndexesForRack(rackIndex);
    for (int clockIndex : indexes) {
        if (m_clocks.at(clockIndex).state == SubClockState::Pending) {
            ++count;
        }
    }
    return count;
}

int SubClockMeasurementPage::rackPassedCount(int rackIndex) const
{
    int count = 0;
    const QVector<int> indexes = clockIndexesForRack(rackIndex);
    for (int clockIndex : indexes) {
        if (m_clocks.at(clockIndex).state == SubClockState::Passed) {
            ++count;
        }
    }
    return count;
}

int SubClockMeasurementPage::rackFailedCount(int rackIndex) const
{
    int count = 0;
    const QVector<int> indexes = clockIndexesForRack(rackIndex);
    for (int clockIndex : indexes) {
        if (m_clocks.at(clockIndex).state == SubClockState::Failed) {
            ++count;
        }
    }
    return count;
}

QString SubClockMeasurementPage::currentSelectionText() const
{
    const int clockIndex = selectedClockIndex();
    if (clockIndex >= 0 && clockIndex < m_clocks.size() && m_clocks.at(clockIndex).active) {
        const SubClockInfo &clock = m_clocks.at(clockIndex);
        return QString::fromUtf8("测试架 %1 · %2 · %3 · %4")
                .arg(clock.rackIndex + 1)
                .arg(slotPointText(clock.row, clock.column, clock.span))
                .arg(shapeText(clock.shape))
                .arg(stateText(clock.state));
    }

    return QString::fromUtf8("测试架 %1 · %2 · 未安装")
            .arg(m_currentRackIndex + 1)
            .arg(slotPointText(m_selectedRow, m_selectedCol, 1));
}

QString SubClockMeasurementPage::rackSummaryText(int rackIndex) const
{
    return QString::fromUtf8("测试架 %1：已安装 %2 点 / 未安装 %3 点，待测 %4，合格 %5，不合格 %6")
            .arg(rackIndex + 1)
            .arg(occupiedPointCount(rackIndex))
            .arg(kRackPointCount - occupiedPointCount(rackIndex))
            .arg(rackPendingCount(rackIndex))
            .arg(rackPassedCount(rackIndex))
            .arg(rackFailedCount(rackIndex));
}
