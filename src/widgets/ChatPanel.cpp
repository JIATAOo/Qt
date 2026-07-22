#include "ChatPanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDateTime>
#include <QKeyEvent>
#include <QDebug>

ChatPanel::ChatPanel(const QString &username, QWidget *parent)
    : QWidget(parent)
    , m_username(username)
{
    setObjectName("chatPanel");
    setFixedWidth(280);
    setupUi();
}

ChatPanel::~ChatPanel() {}

void ChatPanel::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // ──── 标题栏 ────
    auto *header = new QWidget(this);
    header->setObjectName("chatHeader");
    header->setFixedHeight(44);

    auto *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(12, 0, 8, 0);

    m_titleLabel = new QLabel(tr("Chat"), header);
    m_titleLabel->setObjectName("chatTitle");

    m_closeBtn = new QPushButton(tr("X"), header);
    m_closeBtn->setObjectName("chatCloseBtn");
    m_closeBtn->setFixedSize(24, 24);
    m_closeBtn->setCursor(Qt::PointingHandCursor);
    connect(m_closeBtn, &QPushButton::clicked, this, &QWidget::hide);

    headerLayout->addWidget(m_titleLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(m_closeBtn);

    // ──── 消息列表 ────
    m_messageList = new QListWidget(this);
    m_messageList->setObjectName("chatMessageList");
    m_messageList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_messageList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_messageList->setSelectionMode(QAbstractItemView::NoSelection);
    m_messageList->setFocusPolicy(Qt::NoFocus);
    m_messageList->setWordWrap(true);

    // ──── 输入栏 ────
    auto *inputBar = new QWidget(this);
    inputBar->setObjectName("chatInputBar");
    inputBar->setFixedHeight(50);

    auto *inputLayout = new QHBoxLayout(inputBar);
    inputLayout->setContentsMargins(8, 6, 8, 6);
    inputLayout->setSpacing(6);

    m_inputEdit = new QLineEdit(inputBar);
    m_inputEdit->setObjectName("chatInput");
    m_inputEdit->setPlaceholderText(tr("Type a message..."));
    m_inputEdit->setMaxLength(500);

    m_sendBtn = new QPushButton(tr("Send"), inputBar);
    m_sendBtn->setObjectName("chatSendBtn");
    m_sendBtn->setFixedSize(52, 32);
    m_sendBtn->setCursor(Qt::PointingHandCursor);

    inputLayout->addWidget(m_inputEdit);
    inputLayout->addWidget(m_sendBtn);

    connect(m_sendBtn, &QPushButton::clicked, this, &ChatPanel::onSendClicked);
    connect(m_inputEdit, &QLineEdit::returnPressed, this, &ChatPanel::onSendClicked);

    mainLayout->addWidget(header);
    mainLayout->addWidget(m_messageList, 1);
    mainLayout->addWidget(inputBar);
}

void ChatPanel::onSendClicked()
{
    QString text = m_inputEdit->text().trimmed();
    if (text.isEmpty())
        return;

    m_inputEdit->clear();
    addMessage(m_username, text);
    emit messageSent(m_username, text);
}

void ChatPanel::addMessage(const QString &sender, const QString &message)
{
    // 构造带格式的显示文本
    QString timeStr = QDateTime::currentDateTime().toString("hh:mm");
    QString displayText;

    if (sender == m_username) {
        displayText = QString("[%1] You:\n%2").arg(timeStr, message);
    } else {
        displayText = QString("[%1] %2:\n%3").arg(timeStr, sender, message);
    }

    auto *item = new QListWidgetItem(displayText, m_messageList);
    // 自己的消息靠右显示
    if (sender == m_username) {
        item->setTextAlignment(Qt::AlignRight);
    }

    m_messageList->addItem(item);
    m_messageList->scrollToBottom();
}
