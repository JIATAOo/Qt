#include "ParticipantsPanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDebug>

ParticipantsPanel::ParticipantsPanel(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("participantsPanel");
    setFixedWidth(280);
    setupUi();
}

ParticipantsPanel::~ParticipantsPanel() {}

void ParticipantsPanel::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // ──── 标题栏 ────
    auto *header = new QWidget(this);
    header->setObjectName("participantsHeader");
    header->setFixedHeight(44);

    auto *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(12, 0, 8, 0);

    m_titleLabel = new QLabel(tr("Participants"), header);
    m_titleLabel->setObjectName("participantsTitle");

    m_closeBtn = new QPushButton(tr("X"), header);
    m_closeBtn->setObjectName("participantsCloseBtn");
    m_closeBtn->setFixedSize(24, 24);
    m_closeBtn->setCursor(Qt::PointingHandCursor);
    connect(m_closeBtn, &QPushButton::clicked, this, &QWidget::hide);

    headerLayout->addWidget(m_titleLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(m_closeBtn);

    // ──── 参与者列表 ────
    m_participantList = new QListWidget(this);
    m_participantList->setObjectName("participantList");
    m_participantList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_participantList->setSelectionMode(QAbstractItemView::NoSelection);
    m_participantList->setFocusPolicy(Qt::NoFocus);

    mainLayout->addWidget(header);
    mainLayout->addWidget(m_participantList, 1);
}

void ParticipantsPanel::setParticipants(const QStringList &participants)
{
    m_participantList->clear();
    for (const auto &name : participants) {
        addParticipant(name);
    }
}

void ParticipantsPanel::addParticipant(const QString &name)
{
    // 避免重复
    for (int i = 0; i < m_participantList->count(); ++i) {
        if (m_participantList->item(i)->text() == name)
            return;
    }

    auto *item = new QListWidgetItem(name, m_participantList);
    item->setSizeHint(QSize(0, 44));
    m_participantList->addItem(item);
    m_participantList->scrollToBottom();
}

void ParticipantsPanel::removeParticipant(const QString &name)
{
    for (int i = 0; i < m_participantList->count(); ++i) {
        if (m_participantList->item(i)->text() == name) {
            delete m_participantList->takeItem(i);
            return;
        }
    }
}

int ParticipantsPanel::participantCount() const
{
    return m_participantList->count();
}
