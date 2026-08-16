/**
 * SPDX-FileComment: Medical Devices Desktop Frontend
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file LoginWindow.cpp
 * @brief User interface for authentication
 * @version 1.0.0
 * @date 2026-08-16
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 * @LICENSE Apache-2.0
 */

#include "LoginWindow.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QMessageBox>

LoginWindow::LoginWindow(AuthService* authService, QWidget* parent)
    : QWidget(parent), m_authService(authService) {
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setAlignment(Qt::AlignCenter);

    QLabel* titleLabel = new QLabel("<h3>Login to Medical Devices</h3>");
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

    m_userEdit = new QLineEdit(this);
    m_userEdit->setPlaceholderText("Username");
    mainLayout->addWidget(m_userEdit);

    m_passEdit = new QLineEdit(this);
    m_passEdit->setPlaceholderText("Password");
    m_passEdit->setEchoMode(QLineEdit::Password);
    mainLayout->addWidget(m_passEdit);

    m_totpLabel = new QLabel("TOTP Code (2FA):", this);
    m_totpLabel->hide();
    mainLayout->addWidget(m_totpLabel);

    m_totpEdit = new QLineEdit(this);
    m_totpEdit->setPlaceholderText("6-digit code");
    m_totpEdit->hide();
    mainLayout->addWidget(m_totpEdit);

    m_errorLabel = new QLabel("", this);
    m_errorLabel->setStyleSheet("color: red;");
    m_errorLabel->hide();
    mainLayout->addWidget(m_errorLabel);

    m_loginBtn = new QPushButton("Login", this);
    mainLayout->addWidget(m_loginBtn);

    connect(m_loginBtn, &QPushButton::clicked, this, &LoginWindow::onLoginClicked);
    
    // Connect Auth Service
    connect(m_authService, &AuthService::loginSuccess, this, &LoginWindow::onLoginSuccess);
    connect(m_authService, &AuthService::totpRequired, this, &LoginWindow::onTotpRequired);
    connect(m_authService, &AuthService::loginFailed, this, &LoginWindow::onLoginFailed);

    setMinimumSize(350, 250);
}

void LoginWindow::onLoginClicked() {
    m_errorLabel->hide();
    m_loginBtn->setEnabled(false);
    m_loginBtn->setText("Authenticating...");

    QString user = m_userEdit->text();
    QString pass = m_passEdit->text();
    QString totp = m_totpEdit->isVisible() ? m_totpEdit->text() : "";

    m_authService->login(user, pass, totp);
}

void LoginWindow::onLoginSuccess(const QString& access, const QString& refresh, bool mustChangePwd) {
    m_loginBtn->setText("Login");
    m_loginBtn->setEnabled(true);
    
    if (mustChangePwd) {
        QMessageBox::information(this, "Password Change", "You must change your password. (Prompt to be implemented)");
    }
    
    emit loginSuccessful(access, refresh);
}

void LoginWindow::onTotpRequired() {
    m_loginBtn->setText("Verify 2FA");
    m_loginBtn->setEnabled(true);
    m_errorLabel->setText("2FA Required.");
    m_errorLabel->show();
    
    m_totpLabel->show();
    m_totpEdit->show();
    m_totpEdit->setFocus();
}

void LoginWindow::onLoginFailed(const QString& errorMsg) {
    m_loginBtn->setText("Login");
    m_loginBtn->setEnabled(true);
    m_errorLabel->setText(errorMsg);
    m_errorLabel->show();
}
