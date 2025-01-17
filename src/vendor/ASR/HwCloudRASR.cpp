/*
obs-auto-subtitle
 Copyright (C) 2019-2020 Yibai Zhang

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; If not, see <https://www.gnu.org/licenses/>
*/

#include <time.h>
#include <QDebug>
#include <QCryptographicHash>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QUrlQuery>
#include <utility>

#include "HwCloudRASR.h"

using namespace std::placeholders;

HwCloudRASR::HwCloudRASR(const QString &paddle_url, QObject *parent)
	: ASRBase(parent), paddle_url(paddle_url)
{
	connect(&ws, &QWebSocket::connected, this, &HwCloudRASR::onConnected);
	connect(&ws, &QWebSocket::disconnected, this,
		&HwCloudRASR::onDisconnected);
	connect(&ws, &QWebSocket::textMessageReceived, this,
		&HwCloudRASR::onTextMessageReceived);
	connect(this, &HwCloudRASR::haveResult, this, &HwCloudRASR::onResult);
	connect(&ws, SIGNAL(error(QAbstractSocket::SocketError)), this,
		SLOT(onError(QAbstractSocket::SocketError)));

	running = false;
}
static const char *startMsg = "{\"name\": \"test.wav\",\"signal\": \"start\",\"nbest\": 1}";

static const char *endMsg = "{\"name\": \"test.wav\",\"signal\": \"end\",\"nbest\": 1}";

void HwCloudRASR::onStart()
{
	auto uri = QString(paddle_url);
	QNetworkRequest request;
	auto urlStr = QString("wss://") +uri;
	QUrl url(urlStr);
	qDebug() << url.toString();
	request.setUrl(url);
	ws.open(request);
}

void HwCloudRASR::onError(QAbstractSocket::SocketError error)
{
	auto errorCb = getErrorCallback();
	if (errorCb)
		errorCb(ERROR_SOCKET, ws.errorString());
	qDebug() << ws.errorString();
}

void HwCloudRASR::onConnected()
{
	ws.sendTextMessage(startMsg);
	auto connectCb = getConnectedCallback();
	if (connectCb)
		connectCb();
	qDebug() << "WebSocket connected";
	running = true;
	connect(this, &ASRBase::sendAudioMessage, this,
		&HwCloudRASR::onSendAudioMessage);
}

void HwCloudRASR::onDisconnected()
{
	running = false;
	auto disconnectCb = getDisconnectedCallback();
	if (disconnectCb)
		disconnectCb();
	qDebug() << "WebSocket disconnected";
}

void HwCloudRASR::onSendAudioMessage(const char *data, unsigned long size)
{
	if (!running) {
		return;
	}
	ws.sendBinaryMessage(QByteArray::fromRawData((const char *)data, size));
}

void HwCloudRASR::onTextMessageReceived(const QString message)
{
	QJsonDocument doc(QJsonDocument::fromJson(message.toUtf8()));
	auto output = doc["result"].toString();
	emit haveResult(output, ResultType_Middle);
}

void HwCloudRASR::onResult(QString message, int type)
{
	auto callback = getResultCallback();
	if (callback)
		callback(message, type);
}

void HwCloudRASR::onStop()
{
	ws.sendTextMessage(QString(endMsg).toUtf8());
	ws.close();
}

HwCloudRASR::~HwCloudRASR()
{
	stop();
}
