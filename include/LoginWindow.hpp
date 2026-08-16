/**
 * SPDX-FileComment: Medical Devices Desktop Frontend
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file LoginWindow.h
 * @brief User interface for authentication
 * @version 1.0.0
 * @date 2026-08-16
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 * @LICENSE Apache-2.0
 */

#ifndef LOGINWINDOW_HPP
#define LOGINWINDOW_HPP

#include <QWidget>
#include "AuthService.hpp"

class QLineEdit;
class QPushButton;
class QLabel;

class LoginWindow : public QWidget {
    Q_OBJECT
public:
    explicit LoginWindow(AuthService* authService, QWidget* parent = nullptr);

signals:
    void loginSuccessful(const QString& accessToken, const QString& refreshToken);

private slots:
    void onLoginClicked();
    void onLoginSuccess(const QString& access, const QString& refresh, bool mustChangePwd);
    void onTotpRequired();
    void onLoginFailed(const QString& errorMsg);

private:
    AuthService* m_authService;
    QLineEdit* m_userEdit;
    QLineEdit* m_passEdit;
    QLineEdit* m_totpEdit;
    QLabel* m_totpLabel;
    QPushButton* m_loginBtn;
    QLabel* m_errorLabel;
};

#endif // LOGINWINDOW_H
