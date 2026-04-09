#ifndef PREVIEWCARDWIDGET_H
#define PREVIEWCARDWIDGET_H

#include <QFrame>

class QFrame;
class QLabel;
class QMouseEvent;

class PreviewCardWidget : public QFrame
{
    Q_OBJECT
public:
    enum LedState {
        LedOk,
        LedWarning,
        LedOff
    };

    explicit PreviewCardWidget(const QString &moduleId,
                               const QString &iconPath,
                               const QString &title,
                               const QString &meta,
                               const QString &value,
                               const QString &accentColor,
                               QWidget *parent = nullptr);

    QString moduleId() const;
    void setActive(bool active);
    void setMetaText(const QString &text);
    void setValueText(const QString &text);
    void setLedState(LedState state);

signals:
    void clicked(const QString &moduleId);

protected:
    void mousePressEvent(QMouseEvent *event) override;

private:
    void refreshStyle();
    void applyAccent();

    QString m_moduleId;
    QString m_accentColor;
    bool m_active;
    QLabel *m_iconLabel;
    QLabel *m_titleLabel;
    QLabel *m_metaLabel;
    QLabel *m_valueLabel;
    QLabel *m_ledLabel;
    QFrame *m_summaryFrame;
};

#endif // PREVIEWCARDWIDGET_H
