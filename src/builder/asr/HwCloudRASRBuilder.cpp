/*
obs-auto-subtitle
 Copyright (C) 2019-2022 Yibai Zhang

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

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QEventLoop>

#include "HwCloudRASRBuilder.h"

#define HWCLOUD_PROVIDER_ID 0x0002U
#define L_SP_PADDLE "AutoSub.SP.Paddle"

#define PROP_PADDLE_URL "autosub_filter_paddle_url"
#define T_PADDLE_URL obs_module_text("AutoSub.Paddle.Url")


void HwCloudRASRBuilder::getProperties(obs_properties_t *props){    
    obs_properties_add_text(props, PROP_PADDLE_URL, T_PADDLE_URL,
                OBS_TEXT_DEFAULT);    
}

void HwCloudRASRBuilder::showProperties(obs_properties_t *props){
    PROPERTY_SET_VISIBLE(props, PROP_PADDLE_URL);    
}

void HwCloudRASRBuilder::hideProperties(obs_properties_t *props){
    PROPERTY_SET_UNVISIBLE(props, PROP_PADDLE_URL);    
}

void HwCloudRASRBuilder::updateSettings(obs_data_t *settings){
    QString _url = obs_data_get_string(settings, PROP_PADDLE_URL);	
    CHECK_CHANGE_SET_ALL(this->paddle_url, _url, needBuild);    
}

void HwCloudRASRBuilder::getDefaults(obs_data_t *settings){
    (void) settings;
}

ASRBase *HwCloudRASRBuilder::build(){        
    auto asr = new HwCloudRASR(paddle_url);
    return asr;
}

static HwCloudRASRBuilder hwCloudRASRBuilder; 
static ASRBuilderRegister register_hwcloud_asr(&hwCloudRASRBuilder, HWCLOUD_PROVIDER_ID, L_SP_PADDLE);
