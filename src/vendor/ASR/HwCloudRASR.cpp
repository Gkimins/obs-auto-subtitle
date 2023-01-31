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

HwCloudRASR::HwCloudRASR(const QString &region, const QString &project_id,
			 const QString &token, QObject *parent)
	: ASRBase(parent), region(region), project_id(project_id), token(token)
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
static const char *startMsg = "{\n"
			      "  \"name\": \"test.wav\",\n"
			      "  \"signal\": \"start\",\n"
				  "  \"nbest\": 1,\n"
			      "}";

static const char *endMsg = "{\n"
			      "  \"name\": \"test.wav\",\n"
			      "  \"signal\": \"end\",\n"
				  "  \"nbest\": 1,\n"
			      "}";

void HwCloudRASR::onStart()
{
	auto uri = QString(HWCLOUD_SIS_RASR_URI).arg(project_id);
	QNetworkRequest request;
	auto urlStr = QString("ws://") +
		      QString(HWCLOUD_SIS_ENDPOINT).arg(region) + uri;
	QUrl url(urlStr);
	qDebug() << url.toString();
	request.setUrl(url);
	request.setRawHeader("X-Auth-Token", token.toLocal8Bit());
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

QString HwCloudRASR::getProjectId()
{
	return project_id;
}

QString HwCloudRASR::getToken()
{
	return token;
}

HwCloudRASR::~HwCloudRASR()
{
	stop();
}
