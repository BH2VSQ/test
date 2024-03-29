/*
 *Dice! QQ dice robot for TRPG game
 *Copyright (C) 2018 w4123溯洄
 *
 *This program is free software: you can redistribute it and/or modify it under the terms
 *of the GNU Affero General Public License as published by the Free Software Foundation,
 *either version 3 of the License, or (at your option) any later version.
 *
 *This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *See the GNU Affero General Public License for more details.
 *
 *You should have received a copy of the GNU Affero General Public License along with this
 *program. If not, see <http://www.gnu.org/licenses/>.
 */
#include <Windows.h>
#include <string>
#include <iostream>
#include <map>
#include <set>
#include <fstream>
#include <algorithm>
#include <ctime>
#include <thread>
#include <queue>
#include <chrono>
#include <mutex>
#include "APPINFO.h"
#include "RD.h"
#include "CQEVE_ALL.h"
#include "CQTools.h"
#include "CQMsgCode.h"
#include "InitList.h"
/*
TODO:
1. en可变成长检定
2. st多人物卡
3. st人物卡绑定
4. st属性展示，全属性展示以及排序
5. rules规则数据库及相应接口和实现
6. 骰子的最大值和最小值以及对应sc实现
7. jrrp优化
*/

using namespace std;
using namespace CQ;
struct MsgType
{
	long long target = 0;
	string msg = "";
	int type;
	//0-Private 1-Group 2-Discuss
};
queue<MsgType> SendMsgQueue;
mutex mutexMsg;
inline void SendMsg(MsgType msg)
{
	if (msg.type == PrivateMsg)
	{
		sendPrivateMsg(msg.target, msg.msg);
	}
	else if (msg.type == GroupMsg)
	{
		sendGroupMsg(msg.target, msg.msg);
	}
	else if (msg.type == DiscussMsg)
	{
		sendDiscussMsg(msg.target, msg.msg);
	}
}

map<long long, RP> JRRP;
map<long long, int> DefaultDice;
map<pair<long long, long long>, string> GroupName;
map<pair<long long, long long>, string> DiscussName;
map<long long, string> WelcomeMsg;
set<long long> DisabledGroup;
set<long long> DisabledDiscuss;
set<long long> DisabledJRRPGroup;
set<long long> DisabledJRRPDiscuss;
set<long long> DisabledMEGroup;
set<long long> DisabledMEDiscuss;
set<long long> DisabledOBGroup;
set<long long> DisabledOBDiscuss;
Initlist ilInitListGroup;
Initlist ilInitListDiscuss;
struct SourceType {
	SourceType(long long a, int b, long long c) :QQ(a), Type(b), GrouporDiscussID(c) {};
	long long QQ = 0;
	int Type = 0;
	long long GrouporDiscussID = 0;
};
struct comp {
	bool operator()(SourceType a, SourceType b) const {
		return a.QQ < b.QQ;
	}
};
using PropType = map<string, int>;
map<SourceType, PropType, comp> CharacterProp;
//CharacterProp Characters;
multimap<long long, long long> ObserveGroup;
multimap<long long, long long> ObserveDiscuss;
string strFileLoc;
bool Enabled = false;
bool MsgSendThreadStarted = false;
EVE_Enable(__eventEnable)
{
	while (MsgSendThreadStarted)
		Sleep(10);//Wait until the thread terminates
	thread MsgSend
	{
		[&] {
		Enabled = true;
		MsgSendThreadStarted = true;
		while (Enabled)
		{
			MsgType msgMsg;
			mutexMsg.lock();
			if (!SendMsgQueue.empty())
			{
				msgMsg = SendMsgQueue.front();
				SendMsgQueue.pop();
			}
			mutexMsg.unlock();
			if (!msgMsg.msg.empty())
			{
				SendMsg(msgMsg);
			}
			this_thread::sleep_for(chrono::milliseconds(20));
		}
		MsgSendThreadStarted = false;
	}
	};
	MsgSend.detach();
	strFileLoc = getAppDirectory();
	ifstream ifstreamGroupName(strFileLoc + "GroupName.RDconf");
	if (ifstreamGroupName)
	{
		long long QQ;
		long long Group;
		string name;
		while (ifstreamGroupName >> QQ >> Group >> name)
		{
			while (name.find("{space}") != string::npos)name.replace(name.find("{space}"), 7, " ");
			while (name.find("{tab}") != string::npos)name.replace(name.find("{tab}"), 5, "\t");
			while (name.find("{endl}") != string::npos)name.replace(name.find("{endl}"), 6, "\n");
			while (name.find("{enter}") != string::npos)name.replace(name.find("{enter}"), 7, "\r");
			GroupName[make_pair(QQ, Group)] = name;
		}
	}
	ifstreamGroupName.close();
	ifstream ifstreamCharacterProp(strFileLoc + "CharacterProp.RDconf");
	if (ifstreamCharacterProp)
	{
		long long QQ,GrouporDiscussID;
		int Type,Value;
		string SkillName;
		while (ifstreamCharacterProp >> QQ >> Type >> GrouporDiscussID >> SkillName >> Value) {
			CharacterProp[SourceType(QQ, Type, GrouporDiscussID)][SkillName] = Value;
		}
	}
	ifstreamCharacterProp.close();
	ifstream ifstreamDiscussName(strFileLoc + "DiscussName.RDconf");
	if (ifstreamDiscussName)
	{
		long long QQ;
		long long Discuss;
		string name;
		while (ifstreamDiscussName >> QQ >> Discuss >> name)
		{
			while (name.find("{space}") != string::npos)name.replace(name.find("{space}"), 7, " ");
			while (name.find("{tab}") != string::npos)name.replace(name.find("{tab}"), 5, "\t");
			while (name.find("{endl}") != string::npos)name.replace(name.find("{endl}"), 6, "\n");
			while (name.find("{enter}") != string::npos)name.replace(name.find("{enter}"), 7, "\r");
			DiscussName[make_pair(QQ, Discuss)] = name;
		}
	}
	ifstreamDiscussName.close();
	ifstream ifstreamDisabledGroup(strFileLoc + "DisabledGroup.RDconf");
	if (ifstreamDisabledGroup)
	{
		long long Group;
		while (ifstreamDisabledGroup >> Group)
		{
			DisabledGroup.insert(Group);
		}
	}
	ifstreamDisabledGroup.close();
	ifstream ifstreamDisabledDiscuss(strFileLoc + "DisabledDiscuss.RDconf");
	if (ifstreamDisabledDiscuss)
	{
		long long Discuss;
		while (ifstreamDisabledDiscuss >> Discuss)
		{
			DisabledDiscuss.insert(Discuss);
		}
	}
	ifstreamDisabledDiscuss.close();
	ifstream ifstreamDisabledJRRPGroup(strFileLoc + "DisabledJRRPGroup.RDconf");
	if (ifstreamDisabledJRRPGroup)
	{
		long long Group;
		while (ifstreamDisabledJRRPGroup >> Group)
		{
			DisabledJRRPGroup.insert(Group);
		}
	}
	ifstreamDisabledJRRPGroup.close();
	ifstream ifstreamDisabledJRRPDiscuss(strFileLoc + "DisabledJRRPDiscuss.RDconf");
	if (ifstreamDisabledJRRPDiscuss)
	{
		long long Discuss;
		while (ifstreamDisabledJRRPDiscuss >> Discuss)
		{
			DisabledJRRPDiscuss.insert(Discuss);
		}
	}
	ifstreamDisabledJRRPDiscuss.close();
	ifstream ifstreamDisabledMEGroup(strFileLoc + "DisabledMEGroup.RDconf");
	if (ifstreamDisabledMEGroup)
	{
		long long Group;
		while (ifstreamDisabledMEGroup >> Group)
		{
			DisabledMEGroup.insert(Group);
		}
	}
	ifstreamDisabledMEGroup.close();
	ifstream ifstreamDisabledMEDiscuss(strFileLoc + "DisabledMEDiscuss.RDconf");
	if (ifstreamDisabledMEDiscuss)
	{
		long long Discuss;
		while (ifstreamDisabledMEDiscuss >> Discuss)
		{
			DisabledMEDiscuss.insert(Discuss);
		}
	}
	ifstreamDisabledMEDiscuss.close();
	ifstream ifstreamDisabledOBGroup(strFileLoc + "DisabledOBGroup.RDconf");
	if (ifstreamDisabledOBGroup)
	{
		long long Group;
		while (ifstreamDisabledOBGroup >> Group)
		{
			DisabledOBGroup.insert(Group);
		}
	}
	ifstreamDisabledOBGroup.close();
	ifstream ifstreamDisabledOBDiscuss(strFileLoc + "DisabledOBDiscuss.RDconf");
	if (ifstreamDisabledOBDiscuss)
	{
		long long Discuss;
		while (ifstreamDisabledOBDiscuss >> Discuss)
		{
			DisabledOBDiscuss.insert(Discuss);
		}
	}
	ifstreamDisabledOBDiscuss.close();
	ifstream ifstreamObserveGroup(strFileLoc + "ObserveGroup.RDconf");
	if (ifstreamObserveGroup)
	{
		long long Group, QQ;
		while (ifstreamObserveGroup >> Group >> QQ)
		{
			ObserveGroup.insert(make_pair(Group, QQ));
		}
	}
	ifstreamObserveGroup.close();

	ifstream ifstreamObserveDiscuss(strFileLoc + "ObserveDiscuss.RDconf");
	if (ifstreamObserveDiscuss)
	{
		long long Discuss, QQ;
		while (ifstreamObserveDiscuss >> Discuss >> QQ)
		{
			ObserveDiscuss.insert(make_pair(Discuss, QQ));
		}
	}
	ifstreamObserveDiscuss.close();
	ifstream ifstreamJRRP(strFileLoc + "JRRP.RDconf");
	if (ifstreamJRRP)
	{
		long long QQ;
		int Val;	
		string strDate;
		while (ifstreamJRRP >> QQ >> strDate >> Val)
		{
			JRRP[QQ].Date = strDate;
			JRRP[QQ].RPVal = Val;
		}
	}
	ifstreamJRRP.close();
	ifstream ifstreamDefault(strFileLoc + "Default.RDconf");
	if (ifstreamDefault)
	{
		long long QQ;
		int DefVal;
		while (ifstreamDefault >> QQ >> DefVal)
		{
			DefaultDice[QQ] = DefVal;
		}
	}
	ifstreamDefault.close();
	ifstream ifstreamWelcomeMsg(strFileLoc + "WelcomeMsg.RDconf");
	if (ifstreamWelcomeMsg)
	{
		long long GroupID;
		string Msg;
		while (ifstreamWelcomeMsg >> GroupID >> Msg)
		{
			while (Msg.find("{space}") != string::npos)Msg.replace(Msg.find("{space}"), 7, " ");
			while (Msg.find("{tab}") != string::npos)Msg.replace(Msg.find("{tab}"), 5, "\t");
			while (Msg.find("{endl}") != string::npos)Msg.replace(Msg.find("{endl}"), 6, "\n");
			while (Msg.find("{enter}") != string::npos)Msg.replace(Msg.find("{enter}"), 7, "\r");
			WelcomeMsg[GroupID] = Msg;
		}
	}
	ifstreamWelcomeMsg.close();
	return 0;
}


EVE_PrivateMsg_EX(__eventPrivateMsg)
{
	if (eve.isSystem())return;
	init(eve.message);
	init2(eve.message);
	if (eve.message[0] != '.')
		return;
	int intMsgCnt = 1;
	while (eve.message[intMsgCnt] == ' ')
		intMsgCnt++;
	eve.message_block();
	string strNickName = "[" + getStrangerInfo(eve.fromQQ).nick + "]";
	string strLowerMessage = eve.message;
	transform(strLowerMessage.begin(), strLowerMessage.end(), strLowerMessage.begin(), tolower);
	if (strLowerMessage.substr(intMsgCnt, 4) == "help")
	{

		mutexMsg.lock();
		SendMsgQueue.push(MsgType{ eve.fromQQ,strHlpMsg,PrivateMsg });
		mutexMsg.unlock();

	}
	else if (strLowerMessage[intMsgCnt] == 'w')
	{
		intMsgCnt++;
		bool boolDetail = false;
		if (strLowerMessage[intMsgCnt] == 'w')
		{
			intMsgCnt++;
			boolDetail = true;
		}
		while (eve.message[intMsgCnt] == ' ')
			intMsgCnt++;

		string strMainDice;
		string strReason;
		int intTmpMsgCnt;
		for (intTmpMsgCnt = intMsgCnt; intTmpMsgCnt != eve.message.length() && eve.message[intTmpMsgCnt] != ' '; intTmpMsgCnt++)
		{
			if (!isdigit(strLowerMessage[intTmpMsgCnt]) && strLowerMessage[intTmpMsgCnt] != 'd' &&strLowerMessage[intTmpMsgCnt] != 'k'&&strLowerMessage[intTmpMsgCnt] != 'p'&&strLowerMessage[intTmpMsgCnt] != 'b'&&strLowerMessage[intTmpMsgCnt] != 'f'&& strLowerMessage[intTmpMsgCnt] != '+'&&strLowerMessage[intTmpMsgCnt] != '-' && strLowerMessage[intTmpMsgCnt] != '#'&& strLowerMessage[intTmpMsgCnt] != 'a')
			{
				break;
			}
		}
		strMainDice = strLowerMessage.substr(intMsgCnt, intTmpMsgCnt - intMsgCnt);
		while (eve.message[intTmpMsgCnt] == ' ')
			intTmpMsgCnt++;
		strReason = eve.message.substr(intTmpMsgCnt);


		int intTurnCnt = 1;
		if (strMainDice.find("#") != string::npos)
		{
			string strTurnCnt = strMainDice.substr(0, strMainDice.find("#"));
			if (strTurnCnt.empty())
				strTurnCnt = "1";
			strMainDice = strMainDice.substr(strMainDice.find("#") + 1);
			RD rdTurnCnt(strTurnCnt, eve.fromQQ);
			int intRdTurnCntRes = rdTurnCnt.Roll();
			if (intRdTurnCntRes == Value_Err)
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromQQ,strValueErr,PrivateMsg });
				mutexMsg.unlock();
				return;
			}
			else if (intRdTurnCntRes == Input_Err)
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromQQ,strInputErr,PrivateMsg });
				mutexMsg.unlock();
				return;
			}
			else if (intRdTurnCntRes == ZeroDice_Err)
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromQQ,strZeroDiceErr,PrivateMsg });
				mutexMsg.unlock();
				return;
			}
			else if (intRdTurnCntRes == ZeroType_Err)
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromQQ,strZeroTypeErr,PrivateMsg });
				mutexMsg.unlock();
				return;
			}
			else if (intRdTurnCntRes == DiceTooBig_Err)
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromQQ,strDiceTooBigErr,PrivateMsg });
				mutexMsg.unlock();
				return;
			}
			else if (intRdTurnCntRes == TypeTooBig_Err)
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromQQ,strTypeTooBigErr,PrivateMsg });
				mutexMsg.unlock();
				return;
			}
			else if (intRdTurnCntRes == AddDiceVal_Err)
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromQQ,strAddDiceValErr,PrivateMsg });
				mutexMsg.unlock();
				return;
			}
			else if (intRdTurnCntRes != 0)
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromQQ,strUnknownErr,PrivateMsg });
				mutexMsg.unlock();
				return;
			}
			if (rdTurnCnt.intTotal > 10)
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromQQ,strRollTimeExceeded,PrivateMsg });
				mutexMsg.unlock();
				return;
			}
			else if (rdTurnCnt.intTotal <= 0)
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromQQ,strRollTimeErr,PrivateMsg });
				mutexMsg.unlock();
				return;
			}
			intTurnCnt = rdTurnCnt.intTotal;
			if (strTurnCnt.find("d") != string::npos)
			{
				string strTurnNotice = strNickName + "的掷骰轮数: " + rdTurnCnt.FormShortString() + "轮";

				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromQQ,strTurnNotice,PrivateMsg });
				mutexMsg.unlock();

			}
		}
		string strFirstDice = strMainDice.substr(0, strMainDice.find('+') < strMainDice.find('-') ? strMainDice.find('+') : strMainDice.find('-'));
		bool boolAdda10 = true;
		for (auto i : strFirstDice)
		{
			if (!isdigit(i))
			{
				boolAdda10 = false;
				break;
			}
		}
		if (boolAdda10)
			strMainDice.insert(strFirstDice.length(), "a10");
		RD rdMainDice(strMainDice, eve.fromQQ);

		int intFirstTimeRes = rdMainDice.Roll();
		if (intFirstTimeRes == Value_Err)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromQQ,strValueErr,PrivateMsg });
			mutexMsg.unlock();
			return;
		}
		else if (intFirstTimeRes == Input_Err)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromQQ,strInputErr,PrivateMsg });
			mutexMsg.unlock();
			return;
		}
		else if (intFirstTimeRes == ZeroDice_Err)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromQQ,strZeroDiceErr,PrivateMsg });
			mutexMsg.unlock();
			return;
		}
		else if (intFirstTimeRes == ZeroType_Err)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromQQ,strZeroTypeErr,PrivateMsg });
			mutexMsg.unlock();
			return;
		}
		else if (intFirstTimeRes == DiceTooBig_Err)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromQQ,strDiceTooBigErr,PrivateMsg });
			mutexMsg.unlock();
			return;
		}
		else if (intFirstTimeRes == TypeTooBig_Err)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromQQ,strTypeTooBigErr,PrivateMsg });
			mutexMsg.unlock();
			return;
		}
		else if (intFirstTimeRes == AddDiceVal_Err)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromQQ,strAddDiceValErr,PrivateMsg });
			mutexMsg.unlock();
			return;
		}
		else if (intFirstTimeRes != 0)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromQQ,strUnknownErr,PrivateMsg });
			mutexMsg.unlock();
			return;
		}
		if (!boolDetail && intTurnCnt != 1)
		{
			string strAns = strNickName + "骰出了: " + to_string(intTurnCnt) + "次" + rdMainDice.strDice + ": { ";
			if (!strReason.empty())
				strAns.insert(0, "由于" + strReason + " ");
			vector<int> vintExVal;
			while (intTurnCnt--)
			{
				rdMainDice.Roll();
				strAns += to_string(rdMainDice.intTotal);
				if (intTurnCnt != 0)
					strAns += ",";
				if ((rdMainDice.strDice == "D100" || rdMainDice.strDice == "1D100") && (rdMainDice.intTotal <= 5 || rdMainDice.intTotal >= 96))
					vintExVal.push_back(rdMainDice.intTotal);
			}
			strAns += " }";
			if (!vintExVal.empty())
			{
				strAns += ",极值: ";
				for (auto it = vintExVal.cbegin(); it != vintExVal.cend(); it++)
				{
					strAns += to_string(*it);
					if (it != vintExVal.cend() - 1)
						strAns += ",";
				}
			}
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromQQ,strAns,PrivateMsg });
			mutexMsg.unlock();
		}
		else
		{
			while (intTurnCnt--)
			{
				rdMainDice.Roll();
				string strAns = strNickName + "骰出了: " + (boolDetail ? rdMainDice.FormCompleteString() : rdMainDice.FormShortString());
				if (!strReason.empty())
					strAns.insert(0, "由于" + strReason + " ");
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromQQ,strAns,PrivateMsg });
				mutexMsg.unlock();

			}
		}


	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "ti")
	{
		string strAns = strNickName + "的疯狂发作-临时症状:\n";
		TempInsane(strAns);

		mutexMsg.lock();
		SendMsgQueue.push(MsgType{ eve.fromQQ,strAns,PrivateMsg });
		mutexMsg.unlock();

	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "li")
	{
		string strAns = strNickName + "的疯狂发作-总结症状:\n";
		LongInsane(strAns);

		mutexMsg.lock();
		SendMsgQueue.push(MsgType{ eve.fromQQ,strAns,PrivateMsg });
		mutexMsg.unlock();

	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "sc")
	{
		intMsgCnt += 2;
		while (strLowerMessage[intMsgCnt] == ' ')
			intMsgCnt++;
		string SanCost = strLowerMessage.substr(intMsgCnt, eve.message.find(' ', intMsgCnt) - intMsgCnt);
		intMsgCnt += SanCost.length();
		while (strLowerMessage[intMsgCnt] == ' ')
			intMsgCnt++;
		string San;
		while (isdigit(strLowerMessage[intMsgCnt])) {
			San += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (SanCost.empty() || SanCost.find("/") == string::npos)
		{

			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromQQ,strSCInvalid,PrivateMsg });
			mutexMsg.unlock();

			return;
		}
		if (San.empty() && !(CharacterProp.count(SourceType(eve.fromQQ,PrivateT,0)) &&CharacterProp[SourceType(eve.fromQQ,PrivateT,0)].count("理智")))
		{

			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromQQ,strSanInvalid,PrivateMsg });
			mutexMsg.unlock();

			return;
		}
		RD rdSuc(SanCost.substr(0, SanCost.find("/")));
		RD rdFail(SanCost.substr(SanCost.find("/") + 1));
		if (rdSuc.Roll() != 0 || rdFail.Roll() != 0)
		{

			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromQQ,strSCInvalid,PrivateMsg });
			mutexMsg.unlock();

			return;
		}
		if (San.length() >= 3)
		{

			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromQQ,strSanInvalid,PrivateMsg });
			mutexMsg.unlock();

			return;
		}
		int intSan = San.empty() ? CharacterProp[SourceType(eve.fromQQ, PrivateT, 0)]["理智"] : stoi(San);
		if (intSan == 0)
		{

			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromQQ,strSanInvalid,PrivateMsg });
			mutexMsg.unlock();

			return;
		}
		string strAns = strNickName + "的Sancheck:\n1D100=";
		int intTmpRollRes = Randint(1, 100);
		strAns += to_string(intTmpRollRes);

		if (intTmpRollRes <= intSan)
		{
			strAns += " 成功\n你的San值减少" + SanCost.substr(0, SanCost.find("/"));
			RD rdSan(SanCost.substr(0, SanCost.find("/")));
			rdSan.Roll();
			if (SanCost.substr(0, SanCost.find("/")).find("d") != string::npos)
				strAns += "=" + to_string(rdSan.intTotal);
			strAns += +"点,当前剩余" + to_string(max(0, intSan - rdSan.intTotal)) + "点";
			if (San.empty())
			{
				CharacterProp[SourceType(eve.fromQQ, PrivateT, 0)]["理智"] = max(0, intSan - rdSan.intTotal);
			}
		}
		else
		{
			strAns += " 失败\n你的San值减少" + SanCost.substr(SanCost.find("/") + 1);
			RD rdSan(SanCost.substr(SanCost.find("/") + 1));
			rdSan.Roll();
			if (SanCost.substr(SanCost.find("/") + 1).find("d") != string::npos)
				strAns += "=" + to_string(rdSan.intTotal);
			strAns += +"点,当前剩余" + to_string(max(0, intSan - rdSan.intTotal)) + "点";
			if (San.empty())
			{
				CharacterProp[SourceType(eve.fromQQ, PrivateT, 0)]["理智"] = max(0, intSan - rdSan.intTotal);
			}
		}

		mutexMsg.lock();
		SendMsgQueue.push(MsgType{ eve.fromQQ,strAns,PrivateMsg });
		mutexMsg.unlock();

	}
	else if (strLowerMessage.substr(intMsgCnt, 3) == "bot")
	{
		mutexMsg.lock();
		SendMsgQueue.push(MsgType{ eve.fromQQ,Dice_Full_Ver,PrivateMsg });
		mutexMsg.unlock();
		return;
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "en")
	{
		intMsgCnt += 2;
		while (strLowerMessage[intMsgCnt] == ' ')
			intMsgCnt++;
		string strSkillName;
		while (intMsgCnt != eve.message.length() && !isdigit(eve.message[intMsgCnt]) && !isspace(eve.message[intMsgCnt]))
		{
			strSkillName += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
		while (isspace(eve.message[intMsgCnt]))
			intMsgCnt++;
		string strCurrentValue;
		while (isdigit(eve.message[intMsgCnt]))
		{
			strCurrentValue += eve.message[intMsgCnt];
			intMsgCnt++;
		}
		int intCurrentVal;
		if (strCurrentValue.empty())
		{
			if(CharacterProp.count(SourceType(eve.fromQQ, PrivateT, 0)) && CharacterProp[SourceType(eve.fromQQ, PrivateT, 0)].count(strSkillName))
			{
				intCurrentVal = CharacterProp[SourceType(eve.fromQQ, PrivateT, 0)][strSkillName];
			}
			else if (SkillDefaultVal.count(strSkillName))
			{
				intCurrentVal = SkillDefaultVal[strSkillName];
			}
			else
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromQQ,strEnValInvalid,PrivateMsg });
				mutexMsg.unlock();
				return;
			}

		}
		else
		{
			if (strCurrentValue.length() > 3)
			{

				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromQQ,strEnValInvalid,PrivateMsg });
				mutexMsg.unlock();

				return;
			}
			intCurrentVal = stoi(strCurrentValue);
		}

		string strAns = strNickName + "的" + strSkillName + "增强或成长检定:\n1D100=";
		int intTmpRollRes = Randint(1, 100);
		strAns += to_string(intTmpRollRes) + "/" + to_string(intCurrentVal);

		if (intTmpRollRes <= intCurrentVal && intTmpRollRes <= 95)
		{
			strAns += " 失败!\n你的" + (strSkillName.empty() ? "属性或技能值" : strSkillName) + "没有变化!";
		}
		else
		{
			strAns += " 成功!\n你的" + (strSkillName.empty() ? "属性或技能值" : strSkillName) + "增加1D10=";
			int intTmpRollD10 = Randint(1, 10);
			strAns += to_string(intTmpRollD10) + "点,当前为" + to_string(intCurrentVal + intTmpRollD10) + "点";
			if (strCurrentValue.empty())
			{
				CharacterProp[SourceType(eve.fromQQ, PrivateT, 0)][strSkillName] = intCurrentVal + intTmpRollD10;
			}
		}

		mutexMsg.lock();
		SendMsgQueue.push(MsgType{ eve.fromQQ,strAns,PrivateMsg });
		mutexMsg.unlock();

	}
	else if (strLowerMessage.substr(intMsgCnt, 4) == "jrrp")
	{
		char cstrDate[100] = {};
		time_t time_tTime = 0;
		time(&time_tTime);
		tm tmTime;
		localtime_s(&tmTime, &time_tTime);
		strftime(cstrDate, 100, "%F", &tmTime);
		if (JRRP.count(eve.fromQQ) && JRRP[eve.fromQQ].Date == cstrDate)
		{
			string strReply = strNickName + "今天的人品值是:" + to_string(JRRP[eve.fromQQ].RPVal);

			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromQQ,strReply,PrivateMsg });
			mutexMsg.unlock();

		}
		else
		{
			normal_distribution<double> NormalDistribution(60, 15);
			mt19937 Generator(static_cast<unsigned int> (GetCycleCount()));
			int JRRPRes;
			do
			{
				JRRPRes = static_cast<int> (NormalDistribution(Generator));
			} while (JRRPRes <= 0 || JRRPRes > 100);
			JRRP[eve.fromQQ].Date = cstrDate;
			JRRP[eve.fromQQ].RPVal = JRRPRes;
			string strReply(strNickName + "今天的人品值是:" + to_string(JRRP[eve.fromQQ].RPVal));

			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromQQ,strReply,PrivateMsg });
			mutexMsg.unlock();

		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 5) == "rules")
	{
		intMsgCnt += 5;
		while (eve.message[intMsgCnt] == ' ')
			intMsgCnt++;
		string strSearch = eve.message.substr(intMsgCnt);
		for (auto &n : strSearch)
			n = toupper(n);
		for (int i = 0; i != strSearch.length(); ++i)
			if (strSearch[i] == ' ')
			{
				strSearch.erase(strSearch.begin() + i);
				i--;
			}
		strSearch = "||" + strSearch + "||";
		int Loc = strRules.find(strSearch);
		if (Loc != string::npos || strSearch == "||战斗||" || strSearch == "||战斗流程||" || strSearch == "||战斗伤害||")
		{
			if (strSearch == "||战斗||" || strSearch == "||战斗流程||")
			{
				string IMAGE = "[CQ:image,file=";
				string IMAGENAME = "Combat.jpg";
				IMAGE += msg_encode(IMAGENAME, true) + "]";
				int res = sendPrivateMsg(eve.fromQQ, IMAGE);
				if (res < 0)
				{
					mutexMsg.lock();
					SendMsgQueue.push(MsgType{ eve.fromQQ, "未找到对应的规则信息!",PrivateMsg });
					mutexMsg.unlock();
				}
			}
			else if (strSearch == "||战斗伤害||")
			{
				string IMAGE = "[CQ:image,file=";
				string IMAGENAME = "Combat1.jpg";
				IMAGE += msg_encode(IMAGENAME, true) + "]";
				int res = sendPrivateMsg(eve.fromQQ, IMAGE);
				if (res < 0)
				{
					mutexMsg.lock();
					SendMsgQueue.push(MsgType{ eve.fromQQ, "未找到对应的规则信息!",PrivateMsg });
					mutexMsg.unlock();
				}
			}
			else
			{
				int LocStart = strRules.find("]", Loc) + 1;
				string strReply = strRules.substr(LocStart, strRules.find("[", LocStart) - LocStart - 1);

				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromQQ,strReply,PrivateMsg });
				mutexMsg.unlock();

			}
		}
		else
		{

			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromQQ, "未找到对应的规则信息!",PrivateMsg });
			mutexMsg.unlock();

		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "st")
	{
		intMsgCnt += 2;
		while (isspace(strLowerMessage[intMsgCnt]))
			intMsgCnt++;
		if (intMsgCnt == strLowerMessage.length())
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromQQ,strStErr,PrivateMsg });
			mutexMsg.unlock();
			return;
		}
		if (strLowerMessage.substr(intMsgCnt, 3) == "clr")
		{
			if (CharacterProp.count(SourceType(eve.fromQQ, PrivateT, 0)))
			{
				CharacterProp.erase(SourceType(eve.fromQQ, PrivateT, 0));
			}
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromQQ,strPropCleared,PrivateMsg });
			mutexMsg.unlock();
			return;
		}
		else if(strLowerMessage.substr(intMsgCnt, 3) == "del"){
			intMsgCnt += 3;
			while (isspace(strLowerMessage[intMsgCnt]))
				intMsgCnt++;
			string strSkillName;
			while (intMsgCnt != strLowerMessage.length() && !isspace(strLowerMessage[intMsgCnt]) && !(strLowerMessage[intMsgCnt] == '|'))
			{
				strSkillName += strLowerMessage[intMsgCnt];
				intMsgCnt++;
			}
			if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
			if (CharacterProp.count(SourceType(eve.fromQQ, PrivateT, 0)) && CharacterProp[SourceType(eve.fromQQ, PrivateT, 0)].count(strSkillName))
			{
				CharacterProp[SourceType(eve.fromQQ, PrivateT, 0)].erase(strSkillName);
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromQQ,strPropDeleted,PrivateMsg });
				mutexMsg.unlock();
			}
			else
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromQQ,strPropNotFound,PrivateMsg });
				mutexMsg.unlock();
			}
			return;
		}
		else if (strLowerMessage.substr(intMsgCnt, 4) == "show") {
			intMsgCnt += 4;
			while (isspace(strLowerMessage[intMsgCnt]))
				intMsgCnt++;
			string strSkillName;
			while (intMsgCnt != strLowerMessage.length() && !isspace(strLowerMessage[intMsgCnt]) && !(strLowerMessage[intMsgCnt] == '|'))
			{
				strSkillName += strLowerMessage[intMsgCnt];
				intMsgCnt++;
			}
			if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
			if (CharacterProp.count(SourceType(eve.fromQQ, PrivateT, 0)) && CharacterProp[SourceType(eve.fromQQ, PrivateT, 0)].count(strSkillName))
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromQQ,format(strProp,{strNickName,strSkillName,to_string(CharacterProp[SourceType(eve.fromQQ, PrivateT, 0)][strSkillName])}),PrivateMsg });
				mutexMsg.unlock();
			}
			else if (SkillDefaultVal.count(strSkillName))
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromQQ,format(strProp,{ strNickName,strSkillName,to_string(SkillDefaultVal[strSkillName]) }),PrivateMsg });
				mutexMsg.unlock();
			}
			else
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromQQ,strPropNotFound,PrivateMsg });
				mutexMsg.unlock();
			}
			return;
		}
		bool boolError = false;
		while (intMsgCnt != strLowerMessage.length())
		{
			string strSkillName;
			while (intMsgCnt != strLowerMessage.length() && !isdigit(strLowerMessage[intMsgCnt]) && !isspace(strLowerMessage[intMsgCnt]) && strLowerMessage[intMsgCnt] != '='&&strLowerMessage[intMsgCnt] != ':')
			{
				strSkillName += strLowerMessage[intMsgCnt];
				intMsgCnt++;
			}
			if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
			while (isspace(strLowerMessage[intMsgCnt]) || strLowerMessage[intMsgCnt] == '=' || strLowerMessage[intMsgCnt] == ':')intMsgCnt++;
			string strSkillVal;
			while (isdigit(strLowerMessage[intMsgCnt]))
			{
				strSkillVal += strLowerMessage[intMsgCnt];
				intMsgCnt++;
			}
			if (strSkillName.empty() || strSkillVal.empty() || strSkillVal.length() > 3)
			{
				boolError = true;
				break;
			}
			CharacterProp[SourceType(eve.fromQQ, PrivateT, 0)][strSkillName] = stoi(strSkillVal);
			while (isspace(strLowerMessage[intMsgCnt]) || strLowerMessage[intMsgCnt] == '|')intMsgCnt++;
		}
		if (boolError) {
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromQQ,strPropErr,PrivateMsg });
			mutexMsg.unlock();
		}
		else {

			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromQQ,strSetPropSuccess,PrivateMsg });
			mutexMsg.unlock();
		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "me")
	{
		intMsgCnt += 2;
		while (strLowerMessage[intMsgCnt] == ' ')
			intMsgCnt++;
		string strGroupID;
		while (isdigit(strLowerMessage[intMsgCnt]))
		{
			strGroupID += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		while (strLowerMessage[intMsgCnt] == ' ')
			intMsgCnt++;
		string strAction = strLowerMessage.substr(intMsgCnt);

		for (auto i : strGroupID)
		{
			if (!isdigit(i))
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromQQ,strGroupIDInvalid,PrivateMsg });
				mutexMsg.unlock();
				return;
			}
		}
		if (strGroupID.empty())
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromQQ,"群号不能为空!",PrivateMsg });
			mutexMsg.unlock();
			return;
		}
		if (strAction.empty())
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromQQ,"动作不能为空!",PrivateMsg });
			mutexMsg.unlock();
			return;
		}
		long long llGroupID = stoll(strGroupID);
		if (DisabledGroup.count(llGroupID))
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromQQ,strDisabledErr,PrivateMsg });
			mutexMsg.unlock();
			return;
		}
		if (DisabledMEGroup.count(llGroupID))
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromQQ,strMEDisabledErr,PrivateMsg });
			mutexMsg.unlock();
			return;
		}
		pair<long long, long long>pairQQGroup(eve.fromQQ, llGroupID);
		string strReply = "[" + (GroupName.count(pairQQGroup) ? GroupName[pairQQGroup] : getGroupMemberInfo(llGroupID, eve.fromQQ).名片.empty() ? getStrangerInfo(eve.fromQQ).nick : getGroupMemberInfo(llGroupID, eve.fromQQ).名片) + "]" + eve.message.substr(intMsgCnt);
		int intSendRes = sendGroupMsg(llGroupID, strReply);
		if (intSendRes < 0)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromQQ,strSendErr,PrivateMsg });
			mutexMsg.unlock();
		}
		else
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromQQ,"命令执行成功!",PrivateMsg });
			mutexMsg.unlock();
		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 3) == "set")
	{
		intMsgCnt += 3;
		while (strLowerMessage[intMsgCnt] == ' ')
			intMsgCnt++;
		string strDefaultDice = strLowerMessage.substr(intMsgCnt, strLowerMessage.find(" ", intMsgCnt) - intMsgCnt);
		while (strDefaultDice[0] == '0')
			strDefaultDice.erase(strDefaultDice.begin());
		if (strDefaultDice.empty())
			strDefaultDice = "100";
		for (auto charNumElement : strDefaultDice)
			if (!isdigit(charNumElement))
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromQQ,strSetInvalid,PrivateMsg });
				mutexMsg.unlock();
				return;
			}
		if (strDefaultDice.length() > 5)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromQQ,strSetTooBig,PrivateMsg });
			mutexMsg.unlock();
			return;
		}
		int intDefaultDice = stoi(strDefaultDice);
		if (intDefaultDice == 100)
			DefaultDice.erase(eve.fromQQ);
		else
			DefaultDice[eve.fromQQ] = intDefaultDice;
		string strSetSuccessReply = "已将" + strNickName + "的默认骰类型更改为D" + strDefaultDice;
		mutexMsg.lock();
		SendMsgQueue.push(MsgType{ eve.fromQQ,strSetSuccessReply,PrivateMsg });
		mutexMsg.unlock();
	}
	else if (strLowerMessage.substr(intMsgCnt, 5) == "coc6d")
	{
		string strReply = strNickName;
		COC6D(strReply);
		mutexMsg.lock();
		SendMsgQueue.push(MsgType{ eve.fromQQ,strReply,PrivateMsg });
		mutexMsg.unlock();
	}
	else if (strLowerMessage.substr(intMsgCnt, 4) == "coc6")
	{
		intMsgCnt += 4;
		if (strLowerMessage[intMsgCnt] == 's')
			intMsgCnt++;
		while (strLowerMessage[intMsgCnt] == ' ')
			intMsgCnt++;
		string strNum;
		while (isdigit(strLowerMessage[intMsgCnt]))
		{
			strNum += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (strNum.length() > 2)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromQQ,strCharacterTooBig,PrivateMsg });
			mutexMsg.unlock();
			return;
		}
		int intNum = stoi(strNum.empty() ? "1" : strNum);
		if (intNum > 10)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromQQ,strCharacterTooBig,PrivateMsg });
			mutexMsg.unlock();
			return;
		}
		if (intNum == 0)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromQQ,strCharacterCannotBeZero,PrivateMsg });
			mutexMsg.unlock();
			return;
		}
		string strReply = strNickName;
		COC6(strReply, intNum);
		mutexMsg.lock();
		SendMsgQueue.push(MsgType{ eve.fromQQ,strReply,PrivateMsg });
		mutexMsg.unlock();
	}
	else if (strLowerMessage.substr(intMsgCnt, 3) == "dnd")
	{
		intMsgCnt += 3;
		while (strLowerMessage[intMsgCnt] == ' ')
			intMsgCnt++;
		string strNum;
		while (isdigit(strLowerMessage[intMsgCnt]))
		{
			strNum += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (strNum.length() > 2)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromQQ,strCharacterTooBig,PrivateMsg });
			mutexMsg.unlock();
			return;
		}
		int intNum = stoi(strNum.empty() ? "1" : strNum);
		if (intNum > 10)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromQQ,strCharacterTooBig,PrivateMsg });
			mutexMsg.unlock();
			return;
		}
		if (intNum == 0)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromQQ,strCharacterCannotBeZero,PrivateMsg });
			mutexMsg.unlock();
			return;
		}
		string strReply = strNickName;
		DND(strReply, intNum);
		mutexMsg.lock();
		SendMsgQueue.push(MsgType{ eve.fromQQ,strReply,PrivateMsg });
		mutexMsg.unlock();
	}
	else if (strLowerMessage.substr(intMsgCnt, 5) == "coc7d" || strLowerMessage.substr(intMsgCnt, 4) == "cocd")
	{
		string strReply = strNickName;
		COC7D(strReply);
		mutexMsg.lock();
		SendMsgQueue.push(MsgType{ eve.fromQQ,strReply,PrivateMsg });
		mutexMsg.unlock();
	}
	else if (strLowerMessage.substr(intMsgCnt, 3) == "coc")
	{
		intMsgCnt += 3;
		if (strLowerMessage[intMsgCnt] == '7')
			intMsgCnt++;
		if (strLowerMessage[intMsgCnt] == 's')
			intMsgCnt++;
		while (strLowerMessage[intMsgCnt] == ' ')
			intMsgCnt++;
		string strNum;
		while (isdigit(strLowerMessage[intMsgCnt]))
		{
			strNum += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (strNum.length() > 2)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromQQ,strCharacterTooBig,PrivateMsg });
			mutexMsg.unlock();
			return;
		}
		int intNum = stoi(strNum.empty() ? "1" : strNum);
		if (intNum > 10)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromQQ,strCharacterTooBig,PrivateMsg });
			mutexMsg.unlock();
			return;
		}
		if (intNum == 0)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromQQ,strCharacterCannotBeZero,PrivateMsg });
			mutexMsg.unlock();
			return;
		}
		string strReply = strNickName;
		COC7(strReply, intNum);
		mutexMsg.lock();
		SendMsgQueue.push(MsgType{ eve.fromQQ,strReply,PrivateMsg });
		mutexMsg.unlock();
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "ra")
	{
		intMsgCnt += 2;
		RD rdD100("D100");	
		string strSkillName;
		while (isspace(strLowerMessage[intMsgCnt]))intMsgCnt++;
		while (intMsgCnt!=strLowerMessage.length() && !isdigit(strLowerMessage[intMsgCnt]) && !isspace(strLowerMessage[intMsgCnt]) && strLowerMessage[intMsgCnt] != '='&&strLowerMessage[intMsgCnt] != ':')
		{
			strSkillName += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
		while (isspace(strLowerMessage[intMsgCnt]) || strLowerMessage[intMsgCnt] == '=' || strLowerMessage[intMsgCnt] == ':')intMsgCnt++;
		string strSkillVal;
		while (isdigit(strLowerMessage[intMsgCnt]))
		{
			strSkillVal += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		string strReason;
		while (isspace(strLowerMessage[intMsgCnt]))
		{
			intMsgCnt++;
		}
		strReason = eve.message.substr(intMsgCnt);
		int intSkillVal = 0;
		if (strSkillVal.empty())
		{
			if (CharacterProp.count(SourceType(eve.fromQQ, PrivateT, 0)) && CharacterProp[SourceType(eve.fromQQ, PrivateT, 0)].count(strSkillName))
			{
				intSkillVal = CharacterProp[SourceType(eve.fromQQ, PrivateT, 0)][strSkillName];
			}
			else if(SkillDefaultVal.count(strSkillName))
			{
				intSkillVal = SkillDefaultVal[strSkillName];
			}
			else
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromQQ,strUnknownPropErr,PrivateMsg });
				mutexMsg.unlock();
				return;
			}
		}
		else if (strSkillVal.length() > 3)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromQQ,strPropErr,PrivateMsg });
			mutexMsg.unlock();
			return;
		}
		else {
			intSkillVal = stoi(strSkillVal);
		}
		rdD100.Roll();
		string strReply = strNickName + "进行" + strSkillName + "检定: " + rdD100.FormShortString() + "/" + to_string(intSkillVal) + " ";
		if (rdD100.intTotal <= 5)strReply += "大成功";
		else if (rdD100.intTotal <= intSkillVal / 5)strReply += "极难成功";
		else if (rdD100.intTotal <= intSkillVal / 2)strReply += "困难成功";
		else if (rdD100.intTotal <= intSkillVal)strReply += "成功";
		else if (rdD100.intTotal <= 95)strReply += "失败";
		else strReply += "大失败";
		if (!strReason.empty())
		{
			strReply = "由于" + strReason + " " + strReply;
		}
		mutexMsg.lock();
		SendMsgQueue.push(MsgType{ eve.fromQQ,strReply,PrivateMsg });
		mutexMsg.unlock();
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "rc")
	{
		intMsgCnt += 2;
		RD rdD100("D100");
		string strSkillName;
		while (isspace(strLowerMessage[intMsgCnt]))intMsgCnt++;
		while (intMsgCnt != strLowerMessage.length() && !isdigit(strLowerMessage[intMsgCnt]) && !isspace(strLowerMessage[intMsgCnt]) && strLowerMessage[intMsgCnt] != '='&&strLowerMessage[intMsgCnt] != ':')
		{
			strSkillName += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
		while (isspace(strLowerMessage[intMsgCnt]) || strLowerMessage[intMsgCnt] == '=' || strLowerMessage[intMsgCnt] == ':')intMsgCnt++;
		string strSkillVal;
		while (isdigit(strLowerMessage[intMsgCnt]))
		{
			strSkillVal += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		string strReason;
		while (isspace(strLowerMessage[intMsgCnt]))
		{
			intMsgCnt++;
		}
		strReason = eve.message.substr(intMsgCnt);
		int intSkillVal = 0;
		if (strSkillVal.empty())
		{
			if (CharacterProp.count(SourceType(eve.fromQQ, PrivateT, 0)) && CharacterProp[SourceType(eve.fromQQ, PrivateT, 0)].count(strSkillName))
			{
				intSkillVal = CharacterProp[SourceType(eve.fromQQ, PrivateT, 0)][strSkillName];
			}
			else if (SkillDefaultVal.count(strSkillName))
			{
				intSkillVal = SkillDefaultVal[strSkillName];
			}
			else
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromQQ,strUnknownPropErr,PrivateMsg });
				mutexMsg.unlock();
				return;
			}
		}
		else if (strSkillVal.length() > 3)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromQQ,strPropErr,PrivateMsg });
			mutexMsg.unlock();
			return;
		}
		else {
			intSkillVal = stoi(strSkillVal);
		}
		rdD100.Roll();
		string strReply = strNickName + "进行" + strSkillName + "检定: " + rdD100.FormShortString() + "/" + to_string(intSkillVal) + " ";
		if (rdD100.intTotal == 1)strReply += "大成功";
		else if (rdD100.intTotal <= intSkillVal / 5)strReply += "极难成功";
		else if (rdD100.intTotal <= intSkillVal / 2)strReply += "困难成功";
		else if (rdD100.intTotal <= intSkillVal)strReply += "成功";
		else if (rdD100.intTotal <= 95 || (intSkillVal > 50 && rdD100.intTotal != 100))strReply += "失败";
		else strReply += "大失败";
		if (!strReason.empty())
		{
			strReply = "由于" + strReason + " " + strReply;
		}
		mutexMsg.lock();
		SendMsgQueue.push(MsgType{ eve.fromQQ,strReply,PrivateMsg });
		mutexMsg.unlock();
	}
	else if (strLowerMessage[intMsgCnt] == 'r' || strLowerMessage[intMsgCnt] == 'o' || strLowerMessage[intMsgCnt] == 'd')
	{
		intMsgCnt += 1;
		bool boolDetail = true;
		if (eve.message[intMsgCnt] == 's')
		{
			boolDetail = false;
			intMsgCnt++;
		}
		while (eve.message[intMsgCnt] == ' ')
			intMsgCnt++;
		string strMainDice;
		string strReason;
		bool tmpContainD = false;
		int intTmpMsgCnt;
		for (intTmpMsgCnt = intMsgCnt; intTmpMsgCnt != eve.message.length() && eve.message[intTmpMsgCnt] != ' '; intTmpMsgCnt++)
		{
			if (strLowerMessage[intTmpMsgCnt] == 'd' || strLowerMessage[intTmpMsgCnt] == 'p' || strLowerMessage[intTmpMsgCnt] == 'b' || strLowerMessage[intTmpMsgCnt] == '#' || strLowerMessage[intTmpMsgCnt] == 'f' || strLowerMessage[intTmpMsgCnt] == 'a')
				tmpContainD = true;
			if (!isdigit(strLowerMessage[intTmpMsgCnt]) && strLowerMessage[intTmpMsgCnt] != 'd' &&strLowerMessage[intTmpMsgCnt] != 'k'&&strLowerMessage[intTmpMsgCnt] != 'p'&&strLowerMessage[intTmpMsgCnt] != 'b'&&strLowerMessage[intTmpMsgCnt] != 'f'&& strLowerMessage[intTmpMsgCnt] != '+'&&strLowerMessage[intTmpMsgCnt] != '-' && strLowerMessage[intTmpMsgCnt] != '#' && strLowerMessage[intTmpMsgCnt] != 'a')
			{
				break;
			}
		}
		if (tmpContainD)
		{
			strMainDice = strLowerMessage.substr(intMsgCnt, intTmpMsgCnt - intMsgCnt);
			while (eve.message[intTmpMsgCnt] == ' ')
				intTmpMsgCnt++;
			strReason = eve.message.substr(intTmpMsgCnt);
		}
		else
			strReason = eve.message.substr(intMsgCnt);

		int intTurnCnt = 1;
		if (strMainDice.find("#") != string::npos)
		{
			string strTurnCnt = strMainDice.substr(0, strMainDice.find("#"));
			if (strTurnCnt.empty())
				strTurnCnt = "1";
			strMainDice = strMainDice.substr(strMainDice.find("#") + 1);
			RD rdTurnCnt(strTurnCnt, eve.fromQQ);
			int intRdTurnCntRes = rdTurnCnt.Roll();
			if (intRdTurnCntRes == Value_Err)
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromQQ,strValueErr,PrivateMsg });
				mutexMsg.unlock();
				return;
			}
			else if (intRdTurnCntRes == Input_Err)
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromQQ,strInputErr,PrivateMsg });
				mutexMsg.unlock();
				return;
			}
			else if (intRdTurnCntRes == ZeroDice_Err)
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromQQ,strZeroDiceErr,PrivateMsg });
				mutexMsg.unlock();
				return;
			}
			else if (intRdTurnCntRes == ZeroType_Err)
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromQQ,strZeroTypeErr,PrivateMsg });
				mutexMsg.unlock();
				return;
			}
			else if (intRdTurnCntRes == DiceTooBig_Err)
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromQQ,strDiceTooBigErr,PrivateMsg });
				mutexMsg.unlock();
				return;
			}
			else if (intRdTurnCntRes == TypeTooBig_Err)
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromQQ,strTypeTooBigErr,PrivateMsg });
				mutexMsg.unlock();
				return;
			}
			else if (intRdTurnCntRes == AddDiceVal_Err)
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromQQ,strAddDiceValErr,PrivateMsg });
				mutexMsg.unlock();
				return;
			}
			else if (intRdTurnCntRes != 0)
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromQQ,strUnknownErr,PrivateMsg });
				mutexMsg.unlock();
				return;
			}
			if (rdTurnCnt.intTotal > 10)
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromQQ,strRollTimeExceeded,PrivateMsg });
				mutexMsg.unlock();
				return;
			}
			else if (rdTurnCnt.intTotal <= 0)
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromQQ,strRollTimeErr,PrivateMsg });
				mutexMsg.unlock();
				return;
			}
			intTurnCnt = rdTurnCnt.intTotal;
			if (strTurnCnt.find("d") != string::npos)
			{
				string strTurnNotice = strNickName + "的掷骰轮数: " + rdTurnCnt.FormShortString() + "轮";

				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromQQ,strTurnNotice,PrivateMsg });
				mutexMsg.unlock();

			}
		}
		RD rdMainDice(strMainDice, eve.fromQQ);

		int intFirstTimeRes = rdMainDice.Roll();
		if (intFirstTimeRes == Value_Err)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromQQ,strValueErr,PrivateMsg });
			mutexMsg.unlock();
			return;
		}
		else if (intFirstTimeRes == Input_Err)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromQQ,strInputErr,PrivateMsg });
			mutexMsg.unlock();
			return;
		}
		else if (intFirstTimeRes == ZeroDice_Err)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromQQ,strZeroDiceErr,PrivateMsg });
			mutexMsg.unlock();
			return;
		}
		else if (intFirstTimeRes == ZeroType_Err)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromQQ,strZeroTypeErr,PrivateMsg });
			mutexMsg.unlock();
			return;
		}
		else if (intFirstTimeRes == DiceTooBig_Err)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromQQ,strDiceTooBigErr,PrivateMsg });
			mutexMsg.unlock();
			return;
		}
		else if (intFirstTimeRes == TypeTooBig_Err)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromQQ,strTypeTooBigErr,PrivateMsg });
			mutexMsg.unlock();
			return;
		}
		else if (intFirstTimeRes == AddDiceVal_Err)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromQQ,strAddDiceValErr,PrivateMsg });
			mutexMsg.unlock();
			return;
		}
		else if (intFirstTimeRes != 0)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromQQ,strUnknownErr,PrivateMsg });
			mutexMsg.unlock();
			return;
		}
		if (!boolDetail&&intTurnCnt != 1)
		{
			string strAns = strNickName + "骰出了: " + to_string(intTurnCnt) + "次" + rdMainDice.strDice + ": { ";
			if (!strReason.empty())
				strAns.insert(0, "由于" + strReason + " ");
			vector<int> vintExVal;
			while (intTurnCnt--)
			{
				rdMainDice.Roll();
				strAns += to_string(rdMainDice.intTotal);
				if (intTurnCnt != 0)
					strAns += ",";
				if ((rdMainDice.strDice == "D100" || rdMainDice.strDice == "1D100") && (rdMainDice.intTotal <= 5 || rdMainDice.intTotal >= 96))
					vintExVal.push_back(rdMainDice.intTotal);
			}
			strAns += " }";
			if (!vintExVal.empty())
			{
				strAns += ",极值: ";
				for (auto it = vintExVal.cbegin(); it != vintExVal.cend(); it++)
				{
					strAns += to_string(*it);
					if (it != vintExVal.cend() - 1)
						strAns += ",";
				}
			}
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromQQ,strAns,PrivateMsg });
			mutexMsg.unlock();
		}
		else
		{
			while (intTurnCnt--)
			{
				rdMainDice.Roll();
				string strAns = strNickName + "骰出了: " + (boolDetail ? rdMainDice.FormCompleteString() : rdMainDice.FormShortString());
				if (!strReason.empty())
					strAns.insert(0, "由于" + strReason + " ");
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromQQ,strAns,PrivateMsg });
				mutexMsg.unlock();

			}
		}
	}
	else
	{
		if (isalpha(eve.message[intMsgCnt]))
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromQQ,"命令输入错误!",PrivateMsg });
			mutexMsg.unlock();
		}
	}
}
EVE_GroupMsg_EX(__eventGroupMsg)
{
	if (eve.isSystem() || eve.isAnonymous())return;
	init(eve.message);
	while (isspace(eve.message[0]))
		eve.message.erase(eve.message.begin());
	string strAt = "[CQ:at,qq=" + to_string(getLoginQQ()) + "]";
	if (eve.message.substr(0, 6) == "[CQ:at") {
		if (eve.message.substr(0, strAt.length()) == strAt) {
			eve.message = eve.message.substr(strAt.length());
		}
		else {
			return;
		}
	}
	init2(eve.message);
	if (eve.message[0] != '.')
		return;
	int intMsgCnt = 1;
	while (eve.message[intMsgCnt] == ' ')
		intMsgCnt++;
	eve.message_block();
	pair<long long, long long>pairQQGroup(eve.fromQQ, eve.fromGroup);
	string strNickName = "[" + (GroupName.count(pairQQGroup) ? GroupName[pairQQGroup] : (getGroupMemberInfo(eve.fromGroup, eve.fromQQ).名片.empty() ? getStrangerInfo(eve.fromQQ).nick : getGroupMemberInfo(eve.fromGroup, eve.fromQQ).名片)) + "]";
	string strLowerMessage = eve.message;
	transform(strLowerMessage.begin(), strLowerMessage.end(), strLowerMessage.begin(), tolower);
	if (strLowerMessage.substr(intMsgCnt, 3) == "bot")
	{
		intMsgCnt += 3;
		while (strLowerMessage[intMsgCnt] == ' ')
			intMsgCnt++;
		string Command;
		while (intMsgCnt != strLowerMessage.length() && !isdigit(strLowerMessage[intMsgCnt]) && !isspace(strLowerMessage[intMsgCnt]))
		{
			Command += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		while (strLowerMessage[intMsgCnt] == ' ')
			intMsgCnt++;
		string QQNum = strLowerMessage.substr(intMsgCnt, eve.message.find(' ', intMsgCnt) - intMsgCnt);
		if (Command == "on")
		{
			if (QQNum.empty() || QQNum == to_string(getLoginQQ()) || (QQNum.length() == 4 && QQNum == to_string(getLoginQQ() % 10000)))
			{
				if (getGroupMemberInfo(eve.fromGroup, eve.fromQQ).permissions >= 2)
				{
					if (DisabledGroup.count(eve.fromGroup))
					{
						DisabledGroup.erase(eve.fromGroup);
						mutexMsg.lock();
						SendMsgQueue.push(MsgType{ eve.fromGroup,"成功开启本机器人!",GroupMsg });
						mutexMsg.unlock();
					}
					else
					{
						mutexMsg.lock();
						SendMsgQueue.push(MsgType{ eve.fromGroup,"本机器人已经处于开启状态!",GroupMsg });
						mutexMsg.unlock();
					}
				}
				else
				{
					mutexMsg.lock();
					SendMsgQueue.push(MsgType{ eve.fromGroup,strPermissionDeniedErr,GroupMsg });
					mutexMsg.unlock();
				}
			}
		}
		else if (Command == "off")
		{
			if (QQNum.empty() || QQNum == to_string(getLoginQQ()) || (QQNum.length() == 4 && QQNum == to_string(getLoginQQ() % 10000)))
			{
				if (getGroupMemberInfo(eve.fromGroup, eve.fromQQ).permissions >= 2)
				{
					if (!DisabledGroup.count(eve.fromGroup))
					{
						DisabledGroup.insert(eve.fromGroup);
						mutexMsg.lock();
						SendMsgQueue.push(MsgType{ eve.fromGroup,"成功关闭本机器人!",GroupMsg });
						mutexMsg.unlock();
					}
					else
					{
						mutexMsg.lock();
						SendMsgQueue.push(MsgType{ eve.fromGroup,"本机器人已经处于关闭状态!",GroupMsg });
						mutexMsg.unlock();
					}
				}
				else
				{
					mutexMsg.lock();
					SendMsgQueue.push(MsgType{ eve.fromGroup,strPermissionDeniedErr,GroupMsg });
					mutexMsg.unlock();
				}
			}
		}
		else
		{
			if (QQNum.empty() || QQNum == to_string(getLoginQQ()) || (QQNum.length() == 4 && QQNum == to_string(getLoginQQ() % 10000)))
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromGroup,Dice_Full_Ver,GroupMsg });
				mutexMsg.unlock();
			}
		}
		return;
	}
	else if (strLowerMessage.substr(intMsgCnt, 7) == "dismiss")
	{
		intMsgCnt += 7;
		while (strLowerMessage[intMsgCnt] == ' ')
			intMsgCnt++;
		string QQNum;
		while (isdigit(strLowerMessage[intMsgCnt]))
		{
			QQNum += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (QQNum.empty() || (QQNum.length() == 4 && QQNum == to_string(getLoginQQ() % 10000)) || QQNum == to_string(getLoginQQ()))
		{
			if(getGroupMemberInfo(eve.fromGroup,eve.fromQQ).permissions >= 2)
			{
				setGroupLeave(eve.fromGroup);
			}
			else
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromGroup,strPermissionDeniedErr,GroupMsg });
				mutexMsg.unlock();
			}
		}
		return;
	}
	if (DisabledGroup.count(eve.fromGroup))
		return;
	if (strLowerMessage.substr(intMsgCnt, 4) == "help")
	{
		mutexMsg.lock();
		SendMsgQueue.push(MsgType{ eve.fromGroup,strHlpMsg,GroupMsg });
		mutexMsg.unlock();
	}
	else if (strLowerMessage.substr(intMsgCnt, 7) == "welcome") {
		intMsgCnt += 7;
		while (strLowerMessage[intMsgCnt] == ' ')
			intMsgCnt++;
		if (getGroupMemberInfo(eve.fromGroup, eve.fromQQ).permissions >= 2)
		{
			string strWelcomeMsg = eve.message.substr(intMsgCnt);
			if (strWelcomeMsg.empty())
			{
				if (WelcomeMsg.count(eve.fromGroup))
				{
					WelcomeMsg.erase(eve.fromGroup);
					mutexMsg.lock();
					SendMsgQueue.push(MsgType{ eve.fromGroup,strWelcomeMsgClearNotice,GroupMsg });
					mutexMsg.unlock();
				}
				else
				{
					mutexMsg.lock();
					SendMsgQueue.push(MsgType{ eve.fromGroup,strWelcomeMsgClearErr,GroupMsg });
					mutexMsg.unlock();
				}
			}
			else {
				WelcomeMsg[eve.fromGroup] = strWelcomeMsg;
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromGroup,strWelcomeMsgUpdateNotice,GroupMsg });
				mutexMsg.unlock();
			}
		}
		else
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromGroup,strPermissionDeniedErr,GroupMsg });
			mutexMsg.unlock();
		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "st")
	{
		intMsgCnt += 2;
		while (isspace(strLowerMessage[intMsgCnt]))
			intMsgCnt++;
		if (intMsgCnt == strLowerMessage.length())
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromGroup,strStErr,GroupMsg });
			mutexMsg.unlock();
			return;
		}
		if (strLowerMessage.substr(intMsgCnt, 3) == "clr")
		{
			if (CharacterProp.count(SourceType(eve.fromQQ, GroupT, eve.fromGroup)))
			{
				CharacterProp.erase(SourceType(eve.fromQQ, GroupT, eve.fromGroup));
			}
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromGroup,strPropCleared,GroupMsg });
			mutexMsg.unlock();
			return;
		}
		else if (strLowerMessage.substr(intMsgCnt, 3) == "del") {
			intMsgCnt += 3;
			while (isspace(strLowerMessage[intMsgCnt]))
				intMsgCnt++;
			string strSkillName;
			while (intMsgCnt != strLowerMessage.length() && !isspace(strLowerMessage[intMsgCnt]) && !(strLowerMessage[intMsgCnt] == '|'))
			{
				strSkillName += strLowerMessage[intMsgCnt];
				intMsgCnt++;
			}
			if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
			if (CharacterProp.count(SourceType(eve.fromQQ, GroupT, eve.fromGroup)) && CharacterProp[SourceType(eve.fromQQ, GroupT, eve.fromGroup)].count(strSkillName))
			{
				CharacterProp[SourceType(eve.fromQQ, GroupT, eve.fromGroup)].erase(strSkillName);
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromGroup,strPropDeleted,GroupMsg });
				mutexMsg.unlock();
			}
			else
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromGroup,strPropNotFound,GroupMsg });
				mutexMsg.unlock();
			}
			return;
		}
		else if (strLowerMessage.substr(intMsgCnt, 4) == "show") {
			intMsgCnt += 4;
			while (isspace(strLowerMessage[intMsgCnt]))
				intMsgCnt++;
			string strSkillName;
			while (intMsgCnt != strLowerMessage.length() && !isspace(strLowerMessage[intMsgCnt]) && !(strLowerMessage[intMsgCnt] == '|'))
			{
				strSkillName += strLowerMessage[intMsgCnt];
				intMsgCnt++;
			}
			if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
			if (CharacterProp.count(SourceType(eve.fromQQ, GroupT, eve.fromGroup)) && CharacterProp[SourceType(eve.fromQQ, GroupT, eve.fromGroup)].count(strSkillName))
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromGroup,format(strProp,{ strNickName,strSkillName,to_string(CharacterProp[SourceType(eve.fromQQ, GroupT, eve.fromGroup)][strSkillName]) }),GroupMsg });
				mutexMsg.unlock();
			}
			else if (SkillDefaultVal.count(strSkillName))
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromGroup,format(strProp,{ strNickName,strSkillName,to_string(SkillDefaultVal[strSkillName]) }),GroupMsg });
				mutexMsg.unlock();
			}
			else
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromGroup,strPropNotFound,GroupMsg });
				mutexMsg.unlock();
			}
			return;
		}
		bool boolError = false;
		while (intMsgCnt != strLowerMessage.length())
		{
			string strSkillName;
			while (intMsgCnt != strLowerMessage.length() && !isdigit(strLowerMessage[intMsgCnt]) && !isspace(strLowerMessage[intMsgCnt]) && strLowerMessage[intMsgCnt] != '='&&strLowerMessage[intMsgCnt] != ':')
			{
				strSkillName += strLowerMessage[intMsgCnt];
				intMsgCnt++;
			}
			if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
			while (isspace(strLowerMessage[intMsgCnt]) || strLowerMessage[intMsgCnt] == '=' || strLowerMessage[intMsgCnt] == ':')intMsgCnt++;
			string strSkillVal;
			while (isdigit(strLowerMessage[intMsgCnt]))
			{
				strSkillVal += strLowerMessage[intMsgCnt];
				intMsgCnt++;
			}
			if (strSkillName.empty() || strSkillVal.empty() || strSkillVal.length() > 3)
			{
				boolError = true;
				break;
			}
			CharacterProp[SourceType(eve.fromQQ, GroupT, eve.fromGroup)][strSkillName] = stoi(strSkillVal);
			while (isspace(strLowerMessage[intMsgCnt]) || strLowerMessage[intMsgCnt] == '|')intMsgCnt++;
		}
		if (boolError) {
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromGroup,strPropErr,GroupMsg });
			mutexMsg.unlock();
		}
		else {

			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromGroup,strSetPropSuccess,GroupMsg });
			mutexMsg.unlock();
		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "ri")
	{
		intMsgCnt += 2;
		while (isspace(strLowerMessage[intMsgCnt]))intMsgCnt++;
		string strinit = "D20";
		if (strLowerMessage[intMsgCnt] == '+' || strLowerMessage[intMsgCnt] == '-')
		{
			strinit += strLowerMessage[intMsgCnt];
			intMsgCnt++;
			while (isspace(strLowerMessage[intMsgCnt]))intMsgCnt++;
		}
		else if (isdigit(strLowerMessage[intMsgCnt]))
			strinit += '+';
		while (isdigit(strLowerMessage[intMsgCnt]))
		{
			strinit += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		string strname;
		while (isspace(strLowerMessage[intMsgCnt]))
		{
			intMsgCnt++;
		}
		strname = eve.message.substr(intMsgCnt);
		if (strname.empty())
			strname = strNickName;
		else
			strname = "[" + strname + "]";
		RD initdice(strinit);
		int intFirstTimeRes = initdice.Roll();
		if (intFirstTimeRes == Value_Err)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromGroup,strValueErr,GroupMsg });
			mutexMsg.unlock();
			return;
		}
		else if (intFirstTimeRes == Input_Err)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromGroup,strInputErr,GroupMsg });
			mutexMsg.unlock();
			return;
		}
		else if (intFirstTimeRes == ZeroDice_Err)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromGroup,strZeroDiceErr,GroupMsg });
			mutexMsg.unlock();
			return;
		}
		else if (intFirstTimeRes == ZeroType_Err)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromGroup,strZeroTypeErr,GroupMsg });
			mutexMsg.unlock();
			return;
		}
		else if (intFirstTimeRes == DiceTooBig_Err)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromGroup,strDiceTooBigErr,GroupMsg });
			mutexMsg.unlock();
			return;
		}
		else if (intFirstTimeRes == TypeTooBig_Err)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromGroup,strTypeTooBigErr,GroupMsg });
			mutexMsg.unlock();
			return;
		}
		else if (intFirstTimeRes == AddDiceVal_Err)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromGroup,strAddDiceValErr,GroupMsg });
			mutexMsg.unlock();
			return;
		}
		else if (intFirstTimeRes != 0)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromGroup,strUnknownErr,GroupMsg });
			mutexMsg.unlock();
			return;
		}
		ilInitListGroup.insert(eve.fromGroup, initdice.intTotal, strname);
		string strReply;
		strReply = strname + "的先攻骰点：" + strinit + '=' + to_string(initdice.intTotal);
		mutexMsg.lock();
		SendMsgQueue.push(MsgType{ eve.fromGroup,strReply,GroupMsg });
		mutexMsg.unlock();
	}
	else if (strLowerMessage.substr(intMsgCnt, 4) == "init")
	{
		intMsgCnt += 4;
		string strReply;
		while (isspace(strLowerMessage[intMsgCnt]))intMsgCnt++;
		if (strLowerMessage.substr(intMsgCnt, 3) == "clr")
		{
			if (ilInitListGroup.clear(eve.fromGroup))
				strReply = "成功清除先攻记录！";
			else
				strReply = "列表为空！";
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromGroup,strReply,GroupMsg });
			mutexMsg.unlock();
			return;
		}
		ilInitListGroup.show(eve.fromGroup, strReply);
		mutexMsg.lock();
		SendMsgQueue.push(MsgType{ eve.fromGroup,strReply,GroupMsg });
		mutexMsg.unlock();
	}
	else if (strLowerMessage[intMsgCnt] == 'w')
	{
		intMsgCnt++;
		bool boolDetail = false;
		if (strLowerMessage[intMsgCnt] == 'w')
		{
			intMsgCnt++;
			boolDetail = true;
		}
		bool isHidden = false;
		if (strLowerMessage[intMsgCnt] == 'h')
		{
			isHidden = true;
			intMsgCnt += 1;
		}
		while (eve.message[intMsgCnt] == ' ')
			intMsgCnt++;

		string strMainDice;
		string strReason;
		int intTmpMsgCnt;
		for (intTmpMsgCnt = intMsgCnt; intTmpMsgCnt != eve.message.length() && eve.message[intTmpMsgCnt] != ' '; intTmpMsgCnt++)
		{
			if (!isdigit(strLowerMessage[intTmpMsgCnt]) && strLowerMessage[intTmpMsgCnt] != 'd' &&strLowerMessage[intTmpMsgCnt] != 'k'&&strLowerMessage[intTmpMsgCnt] != 'p'&&strLowerMessage[intTmpMsgCnt] != 'b'&&strLowerMessage[intTmpMsgCnt] != 'f'&& strLowerMessage[intTmpMsgCnt] != '+'&&strLowerMessage[intTmpMsgCnt] != '-' && strLowerMessage[intTmpMsgCnt] != '#'&& strLowerMessage[intTmpMsgCnt] != 'a')
			{
				break;
			}
		}
		strMainDice = strLowerMessage.substr(intMsgCnt, intTmpMsgCnt - intMsgCnt);
		while (eve.message[intTmpMsgCnt] == ' ')
			intTmpMsgCnt++;
		strReason = eve.message.substr(intTmpMsgCnt);


		int intTurnCnt = 1;
		if (strMainDice.find("#") != string::npos)
		{
			string strTurnCnt = strMainDice.substr(0, strMainDice.find("#"));
			if (strTurnCnt.empty())
				strTurnCnt = "1";
			strMainDice = strMainDice.substr(strMainDice.find("#") + 1);
			RD rdTurnCnt(strTurnCnt, eve.fromQQ);
			int intRdTurnCntRes = rdTurnCnt.Roll();
			if (intRdTurnCntRes == Value_Err)
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromGroup,strValueErr,GroupMsg });
				mutexMsg.unlock();
				return;
			}
			else if (intRdTurnCntRes == Input_Err)
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromGroup,strInputErr,GroupMsg });
				mutexMsg.unlock();
				return;
			}
			else if (intRdTurnCntRes == ZeroDice_Err)
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromGroup,strZeroDiceErr,GroupMsg });
				mutexMsg.unlock();
				return;
			}
			else if (intRdTurnCntRes == ZeroType_Err)
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromGroup,strZeroTypeErr,GroupMsg });
				mutexMsg.unlock();
				return;
			}
			else if (intRdTurnCntRes == DiceTooBig_Err)
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromGroup,strDiceTooBigErr,GroupMsg });
				mutexMsg.unlock();
				return;
			}
			else if (intRdTurnCntRes == TypeTooBig_Err)
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromGroup,strTypeTooBigErr,GroupMsg });
				mutexMsg.unlock();
				return;
			}
			else if (intRdTurnCntRes == AddDiceVal_Err)
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromGroup,strAddDiceValErr,GroupMsg });
				mutexMsg.unlock();
				return;
			}
			else if (intRdTurnCntRes != 0)
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromGroup,strUnknownErr,GroupMsg });
				mutexMsg.unlock();
				return;
			}
			if (rdTurnCnt.intTotal > 10)
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromGroup,strRollTimeExceeded,GroupMsg });
				mutexMsg.unlock();
				return;
			}
			else if (rdTurnCnt.intTotal <= 0)
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromGroup,strRollTimeErr,GroupMsg });
				mutexMsg.unlock();
				return;
			}
			intTurnCnt = rdTurnCnt.intTotal;
			if (strTurnCnt.find("d") != string::npos)
			{
				string strTurnNotice = strNickName + "的掷骰轮数: " + rdTurnCnt.FormShortString() + "轮";
				if (!isHidden)
				{
					mutexMsg.lock();
					SendMsgQueue.push(MsgType{ eve.fromGroup,strTurnNotice,GroupMsg });
					mutexMsg.unlock();
				}
				else
				{
					strTurnNotice = "在群\"" + getGroupList()[eve.fromGroup] + "\"中 " + strTurnNotice;
					mutexMsg.lock();
					SendMsgQueue.push(MsgType{ eve.fromQQ,strTurnNotice,PrivateMsg });
					mutexMsg.unlock();
					auto range = ObserveGroup.equal_range(eve.fromGroup);
					for (auto it = range.first; it != range.second; it++)
					{
						if (it->second != eve.fromQQ)
						{
							mutexMsg.lock();
							SendMsgQueue.push(MsgType{ it->second,strTurnNotice,PrivateMsg });
							mutexMsg.unlock();
						}
					}
				}


			}
		}
		if (strMainDice.empty())
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromGroup,strEmptyWWDiceErr,GroupMsg });
			mutexMsg.unlock();
			return;
		}
		string strFirstDice = strMainDice.substr(0, strMainDice.find('+') < strMainDice.find('-') ? strMainDice.find('+') : strMainDice.find('-'));
		bool boolAdda10 = true;
		for (auto i : strFirstDice)
		{
			if (!isdigit(i))
			{
				boolAdda10 = false;
				break;
			}
		}
		if (boolAdda10)
			strMainDice.insert(strFirstDice.length(), "a10");
		RD rdMainDice(strMainDice, eve.fromQQ);

		int intFirstTimeRes = rdMainDice.Roll();
		if (intFirstTimeRes == Value_Err)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromGroup,strValueErr,GroupMsg });
			mutexMsg.unlock();
			return;
		}
		else if (intFirstTimeRes == Input_Err)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromGroup,strInputErr,GroupMsg });
			mutexMsg.unlock();
			return;
		}
		else if (intFirstTimeRes == ZeroDice_Err)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromGroup,strZeroDiceErr,GroupMsg });
			mutexMsg.unlock();
			return;
		}
		else if (intFirstTimeRes == ZeroType_Err)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromGroup,strZeroTypeErr,GroupMsg });
			mutexMsg.unlock();
			return;
		}
		else if (intFirstTimeRes == DiceTooBig_Err)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromGroup,strDiceTooBigErr,GroupMsg });
			mutexMsg.unlock();
			return;
		}
		else if (intFirstTimeRes == TypeTooBig_Err)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromGroup,strTypeTooBigErr,GroupMsg });
			mutexMsg.unlock();
			return;
		}
		else if (intFirstTimeRes == AddDiceVal_Err)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromGroup,strAddDiceValErr,GroupMsg });
			mutexMsg.unlock();
			return;
		}
		else if (intFirstTimeRes != 0)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromGroup,strUnknownErr,GroupMsg });
			mutexMsg.unlock();
			return;
		}
		if (!boolDetail && intTurnCnt != 1)
		{
			string strAns = strNickName + "骰出了: " + to_string(intTurnCnt) + "次" + rdMainDice.strDice + ": { ";
			if (!strReason.empty())
				strAns.insert(0, "由于" + strReason + " ");
			vector<int> vintExVal;
			while (intTurnCnt--)
			{
				rdMainDice.Roll();
				strAns += to_string(rdMainDice.intTotal);
				if (intTurnCnt != 0)
					strAns += ",";
				if ((rdMainDice.strDice == "D100" || rdMainDice.strDice == "1D100") && (rdMainDice.intTotal <= 5 || rdMainDice.intTotal >= 96))
					vintExVal.push_back(rdMainDice.intTotal);
			}
			strAns += " }";
			if (!vintExVal.empty())
			{
				strAns += ",极值: ";
				for (auto it = vintExVal.cbegin(); it != vintExVal.cend(); it++)
				{
					strAns += to_string(*it);
					if (it != vintExVal.cend() - 1)
						strAns += ",";
				}
			}
			if (!isHidden)
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromGroup,strAns,GroupMsg });
				mutexMsg.unlock();
			}
			else
			{
				strAns = "在群\"" + getGroupList()[eve.fromGroup] + "\"中 " + strAns;
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromQQ,strAns,PrivateMsg });
				mutexMsg.unlock();
				auto range = ObserveGroup.equal_range(eve.fromGroup);
				for (auto it = range.first; it != range.second; it++)
				{
					if (it->second != eve.fromQQ)
					{
						mutexMsg.lock();
						SendMsgQueue.push(MsgType{ it->second,strAns,PrivateMsg });
						mutexMsg.unlock();
					}
				}
			}

		}
		else
		{
			while (intTurnCnt--)
			{
				rdMainDice.Roll();
				string strAns = strNickName + "骰出了: " + (boolDetail ? rdMainDice.FormCompleteString() : rdMainDice.FormShortString());
				if (!strReason.empty())
					strAns.insert(0, "由于" + strReason + " ");
				if (!isHidden)
				{
					mutexMsg.lock();
					SendMsgQueue.push(MsgType{ eve.fromGroup,strAns,GroupMsg });
					mutexMsg.unlock();
				}
				else
				{
					strAns = "在群\"" + getGroupList()[eve.fromGroup] + "\"中 " + strAns;
					mutexMsg.lock();
					SendMsgQueue.push(MsgType{ eve.fromQQ,strAns,PrivateMsg });
					mutexMsg.unlock();
					auto range = ObserveGroup.equal_range(eve.fromGroup);
					for (auto it = range.first; it != range.second; it++)
					{
						if (it->second != eve.fromQQ)
						{
							mutexMsg.lock();
							SendMsgQueue.push(MsgType{ it->second,strAns,PrivateMsg });
							mutexMsg.unlock();
						}
					}
				}

			}
		}
		if (isHidden)
		{
			string strReply = strNickName + "进行了一次暗骰";
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromGroup,strReply,GroupMsg });
			mutexMsg.unlock();
		}

	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "ob")
	{
		intMsgCnt += 2;
		while (strLowerMessage[intMsgCnt] == ' ')
			intMsgCnt++;
		string Command = strLowerMessage.substr(intMsgCnt, eve.message.find(' ', intMsgCnt) - intMsgCnt);
		if (Command == "on")
		{
			if (getGroupMemberInfo(eve.fromGroup, eve.fromQQ).permissions >= 2)
			{
				if (DisabledOBGroup.count(eve.fromGroup))
				{
					DisabledOBGroup.erase(eve.fromGroup);
					mutexMsg.lock();
					SendMsgQueue.push(MsgType{ eve.fromGroup,"成功在本群中启用旁观模式!",GroupMsg });
					mutexMsg.unlock();
				}
				else
				{
					mutexMsg.lock();
					SendMsgQueue.push(MsgType{ eve.fromGroup, "本群中旁观模式没有被禁用!",GroupMsg });
					mutexMsg.unlock();
				}
			}
			else
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromGroup, "你没有权限执行此命令!",GroupMsg });
				mutexMsg.unlock();
			}
			return;
		}
		else if (Command == "off")
		{
			if (getGroupMemberInfo(eve.fromGroup, eve.fromQQ).permissions >= 2)
			{
				if (!DisabledOBGroup.count(eve.fromGroup))
				{
					DisabledOBGroup.insert(eve.fromGroup);
					ObserveGroup.clear();
					mutexMsg.lock();
					SendMsgQueue.push(MsgType{ eve.fromGroup,"成功在本群中禁用旁观模式!",GroupMsg });
					mutexMsg.unlock();
				}
				else
				{
					mutexMsg.lock();
					SendMsgQueue.push(MsgType{ eve.fromGroup, "本群中旁观模式没有被启用!",GroupMsg });
					mutexMsg.unlock();
				}
			}
			else
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromGroup, "你没有权限执行此命令!",GroupMsg });
				mutexMsg.unlock();
			}
			return;
		}
		if (DisabledOBGroup.count(eve.fromGroup))
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromGroup, "在本群中旁观模式已被禁用!",GroupMsg });
			mutexMsg.unlock();
			return;
		}
		if (Command == "list")
		{
			string Msg = "当前的旁观者有:";
			auto Range = ObserveGroup.equal_range(eve.fromGroup);
			for (auto it = Range.first; it != Range.second; ++it)
			{
				pair<long long, long long> ObGroup;
				ObGroup.first = it->second;
				ObGroup.second = eve.fromGroup;
				Msg += "\n" + (GroupName.count(ObGroup) ? GroupName[ObGroup] : getGroupMemberInfo(eve.fromGroup, it->second).名片.empty() ? getStrangerInfo(it->second).nick : getGroupMemberInfo(eve.fromGroup, it->second).名片) + "(" + to_string(it->second) + ")";
			}
			string strReply = Msg == "当前的旁观者有:" ? "当前暂无旁观者" : Msg;
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromGroup,strReply,GroupMsg });
			mutexMsg.unlock();
		}
		else if (Command == "clr")
		{
			if (getGroupMemberInfo(eve.fromGroup, eve.fromQQ).permissions >= 2)
			{
				ObserveGroup.erase(eve.fromGroup);
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromGroup,"成功删除所有旁观者!",GroupMsg });
				mutexMsg.unlock();
			}
			else
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromGroup,"你没有权限执行此命令!",GroupMsg });
				mutexMsg.unlock();
			}
		}
		else if (Command == "exit")
		{
			auto Range = ObserveGroup.equal_range(eve.fromGroup);
			for (auto it = Range.first; it != Range.second; ++it)
			{
				if (it->second == eve.fromQQ)
				{
					ObserveGroup.erase(it);
					string strReply = strNickName + "成功退出旁观模式!";
					mutexMsg.lock();
					SendMsgQueue.push(MsgType{ eve.fromGroup,strReply,GroupMsg });
					mutexMsg.unlock();
					eve.message_block();
					return;
				}
			}
			string strReply = strNickName + "没有加入旁观模式!";
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromGroup,strReply,GroupMsg });
			mutexMsg.unlock();
		}
		else
		{
			auto Range = ObserveGroup.equal_range(eve.fromGroup);
			for (auto it = Range.first; it != Range.second; ++it)
			{
				if (it->second == eve.fromQQ)
				{
					string strReply = strNickName + "已经处于旁观模式!";
					mutexMsg.lock();
					SendMsgQueue.push(MsgType{ eve.fromGroup,strReply,GroupMsg });
					mutexMsg.unlock();
					eve.message_block();
					return;
				}
			}
			ObserveGroup.insert(make_pair(eve.fromGroup, eve.fromQQ));
			string strReply = strNickName + "成功加入旁观模式!";
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromGroup,strReply,GroupMsg });
			mutexMsg.unlock();
		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "ti")
	{
		string strAns = strNickName + "的疯狂发作-临时症状:\n";
		TempInsane(strAns);
		mutexMsg.lock();
		SendMsgQueue.push(MsgType{ eve.fromGroup,strAns,GroupMsg });
		mutexMsg.unlock();
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "li")
	{
		string strAns = strNickName + "的疯狂发作-总结症状:\n";
		LongInsane(strAns);
		mutexMsg.lock();
		SendMsgQueue.push(MsgType{ eve.fromGroup,strAns,GroupMsg });
		mutexMsg.unlock();
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "sc")
	{
		intMsgCnt += 2;
		while (strLowerMessage[intMsgCnt] == ' ')
			intMsgCnt++;
		string SanCost = strLowerMessage.substr(intMsgCnt, eve.message.find(' ', intMsgCnt) - intMsgCnt);
		intMsgCnt += SanCost.length();
		while (strLowerMessage[intMsgCnt] == ' ')
			intMsgCnt++;
		string San;
		while (isdigit(strLowerMessage[intMsgCnt])) {
			San += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (SanCost.empty() || SanCost.find("/") == string::npos)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromGroup,strSCInvalid,GroupMsg });
			mutexMsg.unlock();
			return;
		}
		if (San.empty() && !(CharacterProp.count(SourceType(eve.fromQQ, GroupT, eve.fromGroup)) && CharacterProp[SourceType(eve.fromQQ, GroupT, eve.fromGroup)].count("理智")))
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromGroup,strSanInvalid,GroupMsg });
			mutexMsg.unlock();
			return;
		}
		RD rdTest1(SanCost.substr(0, SanCost.find("/")));
		RD rdTest2(SanCost.substr(SanCost.find("/") + 1));
		if (rdTest1.Roll() != 0 || rdTest2.Roll() != 0)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromGroup,strSCInvalid,GroupMsg });
			mutexMsg.unlock();
			return;
		}
		if (San.length() >= 3)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromGroup,strSanInvalid,GroupMsg });
			mutexMsg.unlock();
			return;
		}
		int intSan = San.empty() ? CharacterProp[SourceType(eve.fromQQ, GroupT, eve.fromGroup)]["理智"] : stoi(San);
		if (intSan == 0)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromGroup,strSanInvalid,GroupMsg });
			mutexMsg.unlock();
			return;
		}
		string strAns = strNickName + "的Sancheck:\n1D100=";
		int intTmpRollRes = Randint(1, 100);
		strAns += to_string(intTmpRollRes);

		if (intTmpRollRes <= intSan)
		{
			strAns += " 成功\n你的San值减少" + SanCost.substr(0, SanCost.find("/"));
			RD rdSan(SanCost.substr(0, SanCost.find("/")));
			rdSan.Roll();
			if (SanCost.substr(0, SanCost.find("/")).find("d") != string::npos)
				strAns += "=" + to_string(rdSan.intTotal);
			strAns += +"点,当前剩余" + to_string(max(0, intSan - rdSan.intTotal)) + "点";
			if (San.empty())
			{
				CharacterProp[SourceType(eve.fromQQ, GroupT, eve.fromGroup)]["理智"] = max(0, intSan - rdSan.intTotal);
			}
		}
		else
		{
			strAns += " 失败\n你的San值减少" + SanCost.substr(SanCost.find("/") + 1);
			RD rdSan(SanCost.substr(SanCost.find("/") + 1));
			rdSan.Roll();
			if (SanCost.substr(SanCost.find("/") + 1).find("d") != string::npos)
				strAns += "=" + to_string(rdSan.intTotal);
			strAns += +"点,当前剩余" + to_string(max(0, intSan - rdSan.intTotal)) + "点";
			if (San.empty())
			{
				CharacterProp[SourceType(eve.fromQQ, GroupT, eve.fromGroup)]["理智"] = max(0, intSan - rdSan.intTotal);
			}
		}
		mutexMsg.lock();
		SendMsgQueue.push(MsgType{ eve.fromGroup,strAns,GroupMsg });
		mutexMsg.unlock();
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "en")
	{
		intMsgCnt += 2;
		while (strLowerMessage[intMsgCnt] == ' ')
			intMsgCnt++;
		string strSkillName;
		while (intMsgCnt != eve.message.length() && !isdigit(eve.message[intMsgCnt]) && !isspace(eve.message[intMsgCnt]))
		{
			strSkillName += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
		while (isspace(eve.message[intMsgCnt]))
			intMsgCnt++;
		string strCurrentValue;
		while (isdigit(eve.message[intMsgCnt]))
		{
			strCurrentValue += eve.message[intMsgCnt];
			intMsgCnt++;
		}
		int intCurrentVal;
		if (strCurrentValue.empty())
		{
			if (CharacterProp.count(SourceType(eve.fromQQ, GroupT, eve.fromGroup)) && CharacterProp[SourceType(eve.fromQQ, GroupT, eve.fromGroup)].count(strSkillName))
			{
				intCurrentVal = CharacterProp[SourceType(eve.fromQQ, GroupT, eve.fromGroup)][strSkillName];
			}
			else if (SkillDefaultVal.count(strSkillName))
			{
				intCurrentVal = SkillDefaultVal[strSkillName];
			}
			else
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromGroup,strEnValInvalid,GroupMsg });
				mutexMsg.unlock();
				return;
			}

		}
		else
		{
			if (strCurrentValue.length() > 3)
			{

				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromGroup,strEnValInvalid,GroupMsg });
				mutexMsg.unlock();

				return;
			}
			intCurrentVal = stoi(strCurrentValue);
		}

		string strAns = strNickName + "的" + strSkillName + "增强或成长检定:\n1D100=";
		int intTmpRollRes = Randint(1, 100);
		strAns += to_string(intTmpRollRes) + "/" + to_string(intCurrentVal);

		if (intTmpRollRes <= intCurrentVal && intTmpRollRes <= 95)
		{
			strAns += " 失败!\n你的" + (strSkillName.empty() ? "属性或技能值" : strSkillName) + "没有变化!";
		}
		else
		{
			strAns += " 成功!\n你的" + (strSkillName.empty() ? "属性或技能值" : strSkillName) + "增加1D10=";
			int intTmpRollD10 = Randint(1, 10);
			strAns += to_string(intTmpRollD10) + "点,当前为" + to_string(intCurrentVal + intTmpRollD10) + "点";
			if (strCurrentValue.empty())
			{
				CharacterProp[SourceType(eve.fromQQ, GroupT, eve.fromGroup)][strSkillName] = intCurrentVal + intTmpRollD10;
			}
		}
		mutexMsg.lock();
		SendMsgQueue.push(MsgType{ eve.fromGroup,strAns,GroupMsg });
		mutexMsg.unlock();
	}
	else if (strLowerMessage.substr(intMsgCnt, 4) == "jrrp")
	{
		intMsgCnt += 4;
		while (strLowerMessage[intMsgCnt] == ' ')
			intMsgCnt++;
		string Command = strLowerMessage.substr(intMsgCnt, eve.message.find(' ', intMsgCnt) - intMsgCnt);
		if (Command == "on")
		{
			if (getGroupMemberInfo(eve.fromGroup, eve.fromQQ).permissions >= 2)
			{
				if (DisabledJRRPGroup.count(eve.fromGroup))
				{
					DisabledJRRPGroup.erase(eve.fromGroup);
					mutexMsg.lock();
					SendMsgQueue.push(MsgType{ eve.fromGroup,"成功在本群中启用JRRP!",GroupMsg });
					mutexMsg.unlock();
				}
				else
				{
					mutexMsg.lock();
					SendMsgQueue.push(MsgType{ eve.fromGroup, "在本群中JRRP没有被禁用!",GroupMsg });
					mutexMsg.unlock();
				}
			}
			else
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromGroup, strPermissionDeniedErr,GroupMsg });
				mutexMsg.unlock();
			}
			return;
		}
		else if (Command == "off")
		{
			if (getGroupMemberInfo(eve.fromGroup, eve.fromQQ).permissions >= 2)
			{
				if (!DisabledJRRPGroup.count(eve.fromGroup))
				{
					DisabledJRRPGroup.insert(eve.fromGroup);
					mutexMsg.lock();
					SendMsgQueue.push(MsgType{ eve.fromGroup,"成功在本群中禁用JRRP!",GroupMsg });
					mutexMsg.unlock();
				}
				else
				{
					mutexMsg.lock();
					SendMsgQueue.push(MsgType{ eve.fromGroup, "在本群中JRRP没有被启用!",GroupMsg });
					mutexMsg.unlock();
				}
			}
			else
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromGroup, strPermissionDeniedErr,GroupMsg });
				mutexMsg.unlock();
			}
			return;
		}
		if (DisabledJRRPGroup.count(eve.fromGroup))
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromGroup, "在本群中JRRP功能已被禁用",GroupMsg });
			mutexMsg.unlock();
			return;
		}
		char cstrDate[100] = {};
		time_t time_tTime = 0;
		time(&time_tTime);
		tm tmTime;
		localtime_s(&tmTime, &time_tTime);
		strftime(cstrDate, 100, "%F", &tmTime);
		if (JRRP.count(eve.fromQQ) && JRRP[eve.fromQQ].Date == cstrDate)
		{
			string strReply = strNickName + "今天的人品值是:" + to_string(JRRP[eve.fromQQ].RPVal);
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromGroup, strReply,GroupMsg });
			mutexMsg.unlock();
		}
		else
		{
			normal_distribution<double> NormalDistribution(60, 15);
			mt19937 Generator(static_cast<unsigned int> (GetCycleCount()));
			int JRRPRes;
			do
			{
				JRRPRes = static_cast<int> (NormalDistribution(Generator));
			} while (JRRPRes <= 0 || JRRPRes > 100);
			JRRP[eve.fromQQ].Date = cstrDate;
			JRRP[eve.fromQQ].RPVal = JRRPRes;
			string strReply(strNickName + "今天的人品值是:" + to_string(JRRP[eve.fromQQ].RPVal));
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromGroup, strReply,GroupMsg });
			mutexMsg.unlock();
		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "nn")
	{
		intMsgCnt += 2;
		while (eve.message[intMsgCnt] == ' ')
			intMsgCnt++;
		string name = eve.message.substr(intMsgCnt);
		if (name.length() > 50)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromGroup, strNameTooLongErr, GroupMsg });
			mutexMsg.unlock();
			return;
		}
		if (!name.empty())
		{
			GroupName[pairQQGroup] = name;
			name = "[" + name + "]";
			string strReply = "已将" + strNickName + "的名称更改为" + name;
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromGroup, strReply,GroupMsg });
			mutexMsg.unlock();
		}
		else
		{
			if (GroupName.count(pairQQGroup))
			{
				GroupName.erase(pairQQGroup);
				string strReply = "已将" + strNickName + "的名称删除";
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromGroup, strReply,GroupMsg });
				mutexMsg.unlock();
			}
			else
			{
				string strReply = strNickName + strNameDelErr;
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromGroup, strReply,GroupMsg });
				mutexMsg.unlock();
			}
		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 5) == "rules")
	{
		intMsgCnt += 5;
		while (eve.message[intMsgCnt] == ' ')
			intMsgCnt++;
		string strSearch = eve.message.substr(intMsgCnt);
		for (auto &n : strSearch)
			n = toupper(n);
		for (int i = 0; i != strSearch.length(); ++i)
			if (strSearch[i] == ' ')
			{
				strSearch.erase(strSearch.begin() + i);
				i--;
			}
		strSearch = "||" + strSearch + "||";
		int Loc = strRules.find(strSearch);
		if (Loc != string::npos || strSearch == "||战斗||" || strSearch == "||战斗流程||" || strSearch == "||战斗伤害||")
		{
			if (strSearch == "||战斗||" || strSearch == "||战斗流程||")
			{
				string IMAGE = "[CQ:image,file=";
				string IMAGENAME = "Combat.jpg";
				IMAGE += msg_encode(IMAGENAME, true) + "]";
				int res = sendGroupMsg(eve.fromGroup, IMAGE);
				if (res < 0)
				{
					mutexMsg.lock();
					SendMsgQueue.push(MsgType{ eve.fromGroup,"未找到对应的规则信息!",GroupMsg });
					mutexMsg.unlock();
				}
			}
			else if (strSearch == "||战斗伤害||")
			{
				string IMAGE = "[CQ:image,file=";
				string IMAGENAME = "Combat1.jpg";
				IMAGE += msg_encode(IMAGENAME, true) + "]";
				int res = sendGroupMsg(eve.fromGroup, IMAGE);
				if (res < 0)
				{
					mutexMsg.lock();
					SendMsgQueue.push(MsgType{ eve.fromGroup,"未找到对应的规则信息!",GroupMsg });
					mutexMsg.unlock();
				}
			}
			else
			{
				int LocStart = strRules.find("]", Loc) + 1;
				string strReply = strRules.substr(LocStart, strRules.find("[", LocStart) - LocStart - 1);
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromGroup,strReply,GroupMsg });
				mutexMsg.unlock();
			}
		}
		else
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromGroup,"未找到对应的规则信息!",GroupMsg });
			mutexMsg.unlock();
		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "me")
	{
		intMsgCnt += 2;
		while (strLowerMessage[intMsgCnt] == ' ')
			intMsgCnt++;
		string strAction = strLowerMessage.substr(intMsgCnt);
		if (strAction == "on")
		{
			if (getGroupMemberInfo(eve.fromGroup, eve.fromQQ).permissions >= 2)
			{
				if (DisabledMEGroup.count(eve.fromGroup))
				{
					DisabledMEGroup.erase(eve.fromGroup);
					mutexMsg.lock();
					SendMsgQueue.push(MsgType{ eve.fromGroup,"成功在本群中启用.me命令!",GroupMsg });
					mutexMsg.unlock();
				}
				else
				{
					mutexMsg.lock();
					SendMsgQueue.push(MsgType{ eve.fromGroup,"在本群中.me命令没有被禁用!",GroupMsg });
					mutexMsg.unlock();
				}
			}
			else
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromGroup,strPermissionDeniedErr,GroupMsg });
				mutexMsg.unlock();
			}
			return;
		}
		else if (strAction == "off")
		{
			if (getGroupMemberInfo(eve.fromGroup, eve.fromQQ).permissions >= 2)
			{
				if (!DisabledMEGroup.count(eve.fromGroup))
				{
					DisabledMEGroup.insert(eve.fromGroup);
					mutexMsg.lock();
					SendMsgQueue.push(MsgType{ eve.fromGroup,"成功在本群中禁用.me命令!",GroupMsg });
					mutexMsg.unlock();
				}
				else
				{
					mutexMsg.lock();
					SendMsgQueue.push(MsgType{ eve.fromGroup,"在本群中.me命令没有被启用!",GroupMsg });
					mutexMsg.unlock();
				}
			}
			else
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromGroup,strPermissionDeniedErr,GroupMsg });
				mutexMsg.unlock();
			}
			return;
		}
		if (DisabledMEGroup.count(eve.fromGroup))
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromGroup,"在本群中.me命令已被禁用!",GroupMsg });
			mutexMsg.unlock();
			return;
		}
		if (strAction.empty())
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromGroup,"动作不能为空!",GroupMsg });
			mutexMsg.unlock();
			return;
		}
		if (DisabledMEGroup.count(eve.fromGroup))
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromGroup,strMEDisabledErr,GroupMsg });
			mutexMsg.unlock();
			return;
		}
		string strReply = strNickName + eve.message.substr(intMsgCnt);
		mutexMsg.lock();
		SendMsgQueue.push(MsgType{ eve.fromGroup,strReply,GroupMsg });
		mutexMsg.unlock();
	}
	else if (strLowerMessage.substr(intMsgCnt, 3) == "set")
	{
		intMsgCnt += 3;
		while (strLowerMessage[intMsgCnt] == ' ')
			intMsgCnt++;
		string strDefaultDice = strLowerMessage.substr(intMsgCnt, strLowerMessage.find(" ", intMsgCnt) - intMsgCnt);
		while (strDefaultDice[0] == '0')
			strDefaultDice.erase(strDefaultDice.begin());
		if (strDefaultDice.empty())
			strDefaultDice = "100";
		for (auto charNumElement : strDefaultDice)
			if (!isdigit(charNumElement))
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromGroup,strSetInvalid,GroupMsg });
				mutexMsg.unlock();
				return;
			}
		if (strDefaultDice.length() > 5)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromGroup,strSetTooBig,GroupMsg });
			mutexMsg.unlock();
			return;
		}
		int intDefaultDice = stoi(strDefaultDice);
		if (intDefaultDice == 100)
			DefaultDice.erase(eve.fromQQ);
		else
			DefaultDice[eve.fromQQ] = intDefaultDice;
		string strSetSuccessReply = "已将" + strNickName + "的默认骰类型更改为D" + strDefaultDice;
		mutexMsg.lock();
		SendMsgQueue.push(MsgType{ eve.fromGroup,strSetSuccessReply,GroupMsg });
		mutexMsg.unlock();
	}
	else if (strLowerMessage.substr(intMsgCnt, 5) == "coc6d")
	{
		string strReply = strNickName;
		COC6D(strReply);
		mutexMsg.lock();
		SendMsgQueue.push(MsgType{ eve.fromGroup,strReply,GroupMsg });
		mutexMsg.unlock();
	}
	else if (strLowerMessage.substr(intMsgCnt, 4) == "coc6")
	{
		intMsgCnt += 4;
		if (strLowerMessage[intMsgCnt] == 's')
			intMsgCnt++;
		while (strLowerMessage[intMsgCnt] == ' ')
			intMsgCnt++;
		string strNum;
		while (isdigit(strLowerMessage[intMsgCnt]))
		{
			strNum += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (strNum.length() > 2)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromQQ,strCharacterTooBig,PrivateMsg });
			mutexMsg.unlock();
			return;
		}
		int intNum = stoi(strNum.empty() ? "1" : strNum);
		if (intNum > 10)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromGroup,strCharacterTooBig,GroupMsg });
			mutexMsg.unlock();
			return;
		}
		if (intNum == 0)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromGroup,strCharacterCannotBeZero,GroupMsg });
			mutexMsg.unlock();
			return;
		}
		string strReply = strNickName;
		COC6(strReply, intNum);
		mutexMsg.lock();
		SendMsgQueue.push(MsgType{ eve.fromGroup,strReply,GroupMsg });
		mutexMsg.unlock();
	}
	else if (strLowerMessage.substr(intMsgCnt, 3) == "dnd")
	{
		intMsgCnt += 3;
		while (strLowerMessage[intMsgCnt] == ' ')
			intMsgCnt++;
		string strNum;
		while (isdigit(strLowerMessage[intMsgCnt]))
		{
			strNum += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (strNum.length() > 2)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromQQ,strCharacterTooBig,PrivateMsg });
			mutexMsg.unlock();
			return;
		}
		int intNum = stoi(strNum.empty() ? "1" : strNum);
		if (intNum > 10)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromGroup,strCharacterTooBig,GroupMsg });
			mutexMsg.unlock();
			return;
		}
		if (intNum == 0)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromGroup,strCharacterCannotBeZero,GroupMsg });
			mutexMsg.unlock();
			return;
		}
		string strReply = strNickName;
		DND(strReply, intNum);
		mutexMsg.lock();
		SendMsgQueue.push(MsgType{ eve.fromGroup,strReply,GroupMsg });
		mutexMsg.unlock();
	}
	else if (strLowerMessage.substr(intMsgCnt, 5) == "coc7d" || strLowerMessage.substr(intMsgCnt, 4) == "cocd")
	{
		string strReply = strNickName;
		COC7D(strReply);
		mutexMsg.lock();
		SendMsgQueue.push(MsgType{ eve.fromGroup,strReply,GroupMsg });
		mutexMsg.unlock();
	}
	else if (strLowerMessage.substr(intMsgCnt, 3) == "coc")
	{
		intMsgCnt += 3;
		if (strLowerMessage[intMsgCnt] == '7')
			intMsgCnt++;
		if (strLowerMessage[intMsgCnt] == 's')
			intMsgCnt++;
		while (strLowerMessage[intMsgCnt] == ' ')
			intMsgCnt++;
		string strNum;
		while (isdigit(strLowerMessage[intMsgCnt]))
		{
			strNum += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (strNum.length() > 2)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromQQ,strCharacterTooBig,PrivateMsg });
			mutexMsg.unlock();
			return;
		}
		int intNum = stoi(strNum.empty() ? "1" : strNum);
		if (intNum > 10)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromGroup,strCharacterTooBig,GroupMsg });
			mutexMsg.unlock();
			return;
		}
		if (intNum == 0)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromGroup,strCharacterCannotBeZero,GroupMsg });
			mutexMsg.unlock();
			return;
		}
		string strReply = strNickName;
		COC7(strReply, intNum);
		mutexMsg.lock();
		SendMsgQueue.push(MsgType{ eve.fromGroup,strReply,GroupMsg });
		mutexMsg.unlock();
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "ra")
	{
		intMsgCnt += 2;
		RD rdD100("D100");
		string strSkillName;
		while (isspace(strLowerMessage[intMsgCnt]))intMsgCnt++;
		while (intMsgCnt != strLowerMessage.length() && !isdigit(strLowerMessage[intMsgCnt]) && !isspace(strLowerMessage[intMsgCnt]) && strLowerMessage[intMsgCnt] != '='&&strLowerMessage[intMsgCnt] != ':')
		{
			strSkillName += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
		while (isspace(strLowerMessage[intMsgCnt]) || strLowerMessage[intMsgCnt] == '=' || strLowerMessage[intMsgCnt] == ':')intMsgCnt++;
		string strSkillVal;
		while (isdigit(strLowerMessage[intMsgCnt]))
		{
			strSkillVal += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		string strReason;
		while (isspace(strLowerMessage[intMsgCnt]))
		{
			intMsgCnt++;
		}
		strReason = eve.message.substr(intMsgCnt);
		int intSkillVal = 0;
		if (strSkillVal.empty())
		{
			if (CharacterProp.count(SourceType(eve.fromQQ, GroupT, eve.fromGroup)) && CharacterProp[SourceType(eve.fromQQ, GroupT, eve.fromGroup)].count(strSkillName))
			{
				intSkillVal = CharacterProp[SourceType(eve.fromQQ, GroupT, eve.fromGroup)][strSkillName];
			}
			else if (SkillDefaultVal.count(strSkillName))
			{
				intSkillVal = SkillDefaultVal[strSkillName];
			}
			else
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromGroup,strUnknownPropErr,GroupMsg });
				mutexMsg.unlock();
				return;
			}
		}
		else if (strSkillVal.length() > 3)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromGroup,strPropErr,GroupMsg });
			mutexMsg.unlock();
			return;
		}
		else {
			intSkillVal = stoi(strSkillVal);
		}
		rdD100.Roll();
		string strReply = strNickName + "进行" + strSkillName + "检定: " + rdD100.FormShortString() + "/" + to_string(intSkillVal) + " ";
		if (rdD100.intTotal <= 5)strReply += "大成功";
		else if (rdD100.intTotal <= intSkillVal / 5)strReply += "极难成功";
		else if (rdD100.intTotal <= intSkillVal / 2)strReply += "困难成功";
		else if (rdD100.intTotal <= intSkillVal)strReply += "成功";
		else if (rdD100.intTotal <= 95)strReply += "失败";
		else strReply += "大失败";
		if (!strReason.empty())
		{
			strReply = "由于" + strReason + " " + strReply;
		}
		mutexMsg.lock();
		SendMsgQueue.push(MsgType{ eve.fromGroup,strReply,GroupMsg });
		mutexMsg.unlock();
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "rc")
	{
		intMsgCnt += 2;
		RD rdD100("D100");
		string strSkillName;
		while (isspace(strLowerMessage[intMsgCnt]))intMsgCnt++;
		while (intMsgCnt != strLowerMessage.length() && !isdigit(strLowerMessage[intMsgCnt]) && !isspace(strLowerMessage[intMsgCnt]) && strLowerMessage[intMsgCnt] != '='&&strLowerMessage[intMsgCnt] != ':')
		{
			strSkillName += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
		while (isspace(strLowerMessage[intMsgCnt]) || strLowerMessage[intMsgCnt] == '=' || strLowerMessage[intMsgCnt] == ':')intMsgCnt++;
		string strSkillVal;
		while (isdigit(strLowerMessage[intMsgCnt]))
		{
			strSkillVal += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		string strReason;
		while (isspace(strLowerMessage[intMsgCnt]))
		{
			intMsgCnt++;
		}
		strReason = eve.message.substr(intMsgCnt);
		int intSkillVal = 0;
		if (strSkillVal.empty())
		{
			if (CharacterProp.count(SourceType(eve.fromQQ, GroupT, eve.fromGroup)) && CharacterProp[SourceType(eve.fromQQ, GroupT, eve.fromGroup)].count(strSkillName))
			{
				intSkillVal = CharacterProp[SourceType(eve.fromQQ, GroupT, eve.fromGroup)][strSkillName];
			}
			else if (SkillDefaultVal.count(strSkillName))
			{
				intSkillVal = SkillDefaultVal[strSkillName];
			}
			else
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromGroup,strUnknownPropErr,GroupMsg });
				mutexMsg.unlock();
				return;
			}
		}
		else if (strSkillVal.length() > 3)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromGroup,strPropErr,GroupMsg });
			mutexMsg.unlock();
			return;
		}
		else {
			intSkillVal = stoi(strSkillVal);
		}
		rdD100.Roll();
		string strReply = strNickName + "进行" + strSkillName + "检定: " + rdD100.FormShortString() + "/" + to_string(intSkillVal) + " ";
		if (rdD100.intTotal == 1)strReply += "大成功";
		else if (rdD100.intTotal <= intSkillVal / 5)strReply += "极难成功";
		else if (rdD100.intTotal <= intSkillVal / 2)strReply += "困难成功";
		else if (rdD100.intTotal <= intSkillVal)strReply += "成功";
		else if (rdD100.intTotal <= 95 || (intSkillVal > 50 && rdD100.intTotal != 100))strReply += "失败";
		else strReply += "大失败";
		if (!strReason.empty())
		{
			strReply = "由于" + strReason + " " + strReply;
		}
		mutexMsg.lock();
		SendMsgQueue.push(MsgType{ eve.fromGroup,strReply,GroupMsg });
		mutexMsg.unlock();
	}
	else if (strLowerMessage[intMsgCnt] == 'r' || strLowerMessage[intMsgCnt] == 'o' || strLowerMessage[intMsgCnt] == 'h' || strLowerMessage[intMsgCnt] == 'd')
	{
		bool isHidden = false;
		if (strLowerMessage[intMsgCnt] == 'h')
			isHidden = true;
		intMsgCnt += 1;
		bool boolDetail = true;
		if (eve.message[intMsgCnt] == 's')
		{
			boolDetail = false;
			intMsgCnt++;
		}
		if (strLowerMessage[intMsgCnt] == 'h')
		{
			isHidden = true;
			intMsgCnt += 1;
		}
		while (eve.message[intMsgCnt] == ' ')
			intMsgCnt++;
		string strMainDice;
		string strReason;
		bool tmpContainD = false;
		int intTmpMsgCnt;
		for (intTmpMsgCnt = intMsgCnt; intTmpMsgCnt != eve.message.length() && eve.message[intTmpMsgCnt] != ' '; intTmpMsgCnt++)
		{
			if (strLowerMessage[intTmpMsgCnt] == 'd' || strLowerMessage[intTmpMsgCnt] == 'p' || strLowerMessage[intTmpMsgCnt] == 'b' || strLowerMessage[intTmpMsgCnt] == '#' || strLowerMessage[intTmpMsgCnt] == 'f' || strLowerMessage[intTmpMsgCnt] == 'a')
				tmpContainD = true;
			if (!isdigit(strLowerMessage[intTmpMsgCnt]) && strLowerMessage[intTmpMsgCnt] != 'd' &&strLowerMessage[intTmpMsgCnt] != 'k'&&strLowerMessage[intTmpMsgCnt] != 'p'&&strLowerMessage[intTmpMsgCnt] != 'b'&&strLowerMessage[intTmpMsgCnt] != 'f'&& strLowerMessage[intTmpMsgCnt] != '+'&&strLowerMessage[intTmpMsgCnt] != '-' && strLowerMessage[intTmpMsgCnt] != '#' && strLowerMessage[intTmpMsgCnt] != 'a')
			{
				break;
			}
		}
		if (tmpContainD)
		{
			strMainDice = strLowerMessage.substr(intMsgCnt, intTmpMsgCnt - intMsgCnt);
			while (eve.message[intTmpMsgCnt] == ' ')
				intTmpMsgCnt++;
			strReason = eve.message.substr(intTmpMsgCnt);
		}
		else
			strReason = eve.message.substr(intMsgCnt);

		int intTurnCnt = 1;
		if (strMainDice.find("#") != string::npos)
		{
			string strTurnCnt = strMainDice.substr(0, strMainDice.find("#"));
			if (strTurnCnt.empty())
				strTurnCnt = "1";
			strMainDice = strMainDice.substr(strMainDice.find("#") + 1);
			RD rdTurnCnt(strTurnCnt, eve.fromQQ);
			int intRdTurnCntRes = rdTurnCnt.Roll();
			if (intRdTurnCntRes == Value_Err)
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromGroup,strValueErr,GroupMsg });
				mutexMsg.unlock();
				return;
			}
			else if (intRdTurnCntRes == Input_Err)
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromGroup,strInputErr,GroupMsg });
				mutexMsg.unlock();
				return;
			}
			else if (intRdTurnCntRes == ZeroDice_Err)
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromGroup,strZeroDiceErr,GroupMsg });
				mutexMsg.unlock();
				return;
			}
			else if (intRdTurnCntRes == ZeroType_Err)
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromGroup,strZeroTypeErr,GroupMsg });
				mutexMsg.unlock();
				return;
			}
			else if (intRdTurnCntRes == DiceTooBig_Err)
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromGroup,strDiceTooBigErr,GroupMsg });
				mutexMsg.unlock();
				return;
			}
			else if (intRdTurnCntRes == TypeTooBig_Err)
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromGroup,strTypeTooBigErr,GroupMsg });
				mutexMsg.unlock();
				return;
			}
			else if (intRdTurnCntRes == AddDiceVal_Err)
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromGroup,strAddDiceValErr,GroupMsg });
				mutexMsg.unlock();
				return;
			}
			else if (intRdTurnCntRes != 0)
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromGroup,strUnknownErr,GroupMsg });
				mutexMsg.unlock();
				return;
			}
			if (rdTurnCnt.intTotal > 10)
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromGroup,strRollTimeExceeded,GroupMsg });
				mutexMsg.unlock();
				return;
			}
			else if (rdTurnCnt.intTotal <= 0)
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromGroup,strRollTimeErr,GroupMsg });
				mutexMsg.unlock();
				return;
			}
			intTurnCnt = rdTurnCnt.intTotal;
			if (strTurnCnt.find("d") != string::npos)
			{
				string strTurnNotice = strNickName + "的掷骰轮数: " + rdTurnCnt.FormShortString() + "轮";
				if (!isHidden)
				{
					mutexMsg.lock();
					SendMsgQueue.push(MsgType{ eve.fromGroup,strTurnNotice,GroupMsg });
					mutexMsg.unlock();
				}
				else
				{
					strTurnNotice = "在群\"" + getGroupList()[eve.fromGroup] + "\"中 " + strTurnNotice;
					mutexMsg.lock();
					SendMsgQueue.push(MsgType{ eve.fromQQ,strTurnNotice,PrivateMsg });
					mutexMsg.unlock();
					auto range = ObserveGroup.equal_range(eve.fromGroup);
					for (auto it = range.first; it != range.second; it++)
					{
						if (it->second != eve.fromQQ)
						{
							mutexMsg.lock();
							SendMsgQueue.push(MsgType{ it->second,strTurnNotice,PrivateMsg });
							mutexMsg.unlock();
						}
					}
				}


			}
		}
		RD rdMainDice(strMainDice, eve.fromQQ);

		int intFirstTimeRes = rdMainDice.Roll();
		if (intFirstTimeRes == Value_Err)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromGroup,strValueErr,GroupMsg });
			mutexMsg.unlock();
			return;
		}
		else if (intFirstTimeRes == Input_Err)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromGroup,strInputErr,GroupMsg });
			mutexMsg.unlock();
			return;
		}
		else if (intFirstTimeRes == ZeroDice_Err)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromGroup,strZeroDiceErr,GroupMsg });
			mutexMsg.unlock();
			return;
		}
		else if (intFirstTimeRes == ZeroType_Err)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromGroup,strZeroTypeErr,GroupMsg });
			mutexMsg.unlock();
			return;
		}
		else if (intFirstTimeRes == DiceTooBig_Err)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromGroup,strDiceTooBigErr,GroupMsg });
			mutexMsg.unlock();
			return;
		}
		else if (intFirstTimeRes == TypeTooBig_Err)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromGroup,strTypeTooBigErr,GroupMsg });
			mutexMsg.unlock();
			return;
		}
		else if (intFirstTimeRes == AddDiceVal_Err)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromGroup,strAddDiceValErr,GroupMsg });
			mutexMsg.unlock();
			return;
		}
		else if (intFirstTimeRes != 0)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromGroup,strUnknownErr,GroupMsg });
			mutexMsg.unlock();
			return;
		}
		if (!boolDetail&&intTurnCnt != 1)
		{
			string strAns = strNickName + "骰出了: " + to_string(intTurnCnt) + "次" + rdMainDice.strDice + ": { ";
			if (!strReason.empty())
				strAns.insert(0, "由于" + strReason + " ");
			vector<int> vintExVal;
			while (intTurnCnt--)
			{
				rdMainDice.Roll();
				strAns += to_string(rdMainDice.intTotal);
				if (intTurnCnt != 0)
					strAns += ",";
				if ((rdMainDice.strDice == "D100" || rdMainDice.strDice == "1D100") && (rdMainDice.intTotal <= 5 || rdMainDice.intTotal >= 96))
					vintExVal.push_back(rdMainDice.intTotal);
			}
			strAns += " }";
			if (!vintExVal.empty())
			{
				strAns += ",极值: ";
				for (auto it = vintExVal.cbegin(); it != vintExVal.cend(); it++)
				{
					strAns += to_string(*it);
					if (it != vintExVal.cend() - 1)
						strAns += ",";
				}
			}
			if (!isHidden)
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromGroup,strAns,GroupMsg });
				mutexMsg.unlock();
			}
			else
			{
				strAns = "在群\"" + getGroupList()[eve.fromGroup] + "\"中 " + strAns;
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromQQ,strAns,PrivateMsg });
				mutexMsg.unlock();
				auto range = ObserveGroup.equal_range(eve.fromGroup);
				for (auto it = range.first; it != range.second; it++)
				{
					if (it->second != eve.fromQQ)
					{
						mutexMsg.lock();
						SendMsgQueue.push(MsgType{ it->second,strAns,PrivateMsg });
						mutexMsg.unlock();
					}
				}
			}

		}
		else
		{
			while (intTurnCnt--)
			{
				rdMainDice.Roll();
				string strAns = strNickName + "骰出了: " + (boolDetail ? rdMainDice.FormCompleteString() : rdMainDice.FormShortString());
				if (!strReason.empty())
					strAns.insert(0, "由于" + strReason + " ");
				if (!isHidden)
				{
					mutexMsg.lock();
					SendMsgQueue.push(MsgType{ eve.fromGroup,strAns,GroupMsg });
					mutexMsg.unlock();
				}
				else
				{
					strAns = "在群\"" + getGroupList()[eve.fromGroup] + "\"中 " + strAns;
					mutexMsg.lock();
					SendMsgQueue.push(MsgType{ eve.fromQQ,strAns,PrivateMsg });
					mutexMsg.unlock();
					auto range = ObserveGroup.equal_range(eve.fromGroup);
					for (auto it = range.first; it != range.second; it++)
					{
						if (it->second != eve.fromQQ)
						{
							mutexMsg.lock();
							SendMsgQueue.push(MsgType{ it->second,strAns,PrivateMsg });
							mutexMsg.unlock();
						}
					}
				}

			}
		}
		if (isHidden)
		{
			string strReply = strNickName + "进行了一次暗骰";
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromGroup,strReply,GroupMsg });
			mutexMsg.unlock();
		}
	}
	else
	{
		if (isalpha(eve.message[intMsgCnt]))
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromGroup,"命令输入错误!",GroupMsg });
			mutexMsg.unlock();
		}
	}
}
EVE_DiscussMsg_EX(__eventDiscussMsg)
{
	if (eve.isSystem())return;
	init(eve.message);
	string strAt = "[CQ:at,qq=" + to_string(getLoginQQ()) + "]";
	if (eve.message.substr(0, 6) == "[CQ:at") {
		if (eve.message.substr(0, strAt.length()) == strAt) {
			eve.message = eve.message.substr(strAt.length() + 1);
		}
		else {
			return;
		}
	}
	init2(eve.message);
	if (eve.message[0] != '.')
		return;
	int intMsgCnt = 1;
	while (eve.message[intMsgCnt] == ' ')
		intMsgCnt++;
	eve.message_block();
	pair<long long, long long>pairQQDiscuss(eve.fromQQ, eve.fromDiscuss);
	string strNickName = "[" + (DiscussName.count(pairQQDiscuss) ? DiscussName[pairQQDiscuss] : getStrangerInfo(eve.fromQQ).nick) + "]";
	string strLowerMessage = eve.message;
	transform(strLowerMessage.begin(), strLowerMessage.end(), strLowerMessage.begin(), tolower);
	if (strLowerMessage.substr(intMsgCnt, 3) == "bot")
	{
		intMsgCnt += 3;
		while (strLowerMessage[intMsgCnt] == ' ')
			intMsgCnt++;
		string Command;
		while (intMsgCnt != strLowerMessage.length() && !isdigit(strLowerMessage[intMsgCnt]) && !isspace(strLowerMessage[intMsgCnt]))
		{
			Command += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		while (strLowerMessage[intMsgCnt] == ' ')
			intMsgCnt++;
		string QQNum = strLowerMessage.substr(intMsgCnt, eve.message.find(' ', intMsgCnt) - intMsgCnt);
		if (Command == "on")
		{
			if (QQNum.empty() || QQNum == to_string(getLoginQQ()) || (QQNum.length() == 4 && QQNum == to_string(getLoginQQ() % 10000)))
			{
				if (DisabledDiscuss.count(eve.fromDiscuss))
				{
					DisabledDiscuss.erase(eve.fromDiscuss);
					mutexMsg.lock();
					SendMsgQueue.push(MsgType{ eve.fromDiscuss,"成功开启本机器人!",DiscussMsg });
					mutexMsg.unlock();
				}
				else
				{
					mutexMsg.lock();
					SendMsgQueue.push(MsgType{ eve.fromDiscuss,"本机器人已经处于开启状态!",DiscussMsg });
					mutexMsg.unlock();
				}
			}
		}
		else if (Command == "off")
		{
			if (QQNum.empty() || QQNum == to_string(getLoginQQ()) || (QQNum.length() == 4 && QQNum == to_string(getLoginQQ() % 10000)))
			{
				if (!DisabledDiscuss.count(eve.fromDiscuss))
				{
					DisabledDiscuss.insert(eve.fromDiscuss);
					mutexMsg.lock();
					SendMsgQueue.push(MsgType{ eve.fromDiscuss,"成功关闭本机器人!",DiscussMsg });
					mutexMsg.unlock();
				}
				else
				{
					mutexMsg.lock();
					SendMsgQueue.push(MsgType{ eve.fromDiscuss,"本机器人已经处于关闭状态!",DiscussMsg });
					mutexMsg.unlock();
				}
			}
		}
		else
		{
			if (QQNum.empty() || QQNum == to_string(getLoginQQ()) || (QQNum.length() == 4 && QQNum == to_string(getLoginQQ() % 10000)))
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromDiscuss, Dice_Full_Ver,DiscussMsg });
				mutexMsg.unlock();
			}
		}
		return;
	}
	else if (strLowerMessage.substr(intMsgCnt, 7) == "dismiss")
	{
		intMsgCnt += 7;
		while (strLowerMessage[intMsgCnt] == ' ')
			intMsgCnt++;
		string QQNum;
		while (isdigit(strLowerMessage[intMsgCnt]))
		{
			QQNum += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (QQNum.empty() || (QQNum.length() == 4 && QQNum == to_string(getLoginQQ() % 10000)) || QQNum == to_string(getLoginQQ()))
		{
			setDiscussLeave(eve.fromDiscuss);
		}
		return;
	}
	if (DisabledDiscuss.count(eve.fromDiscuss))
		return;
	if (strLowerMessage.substr(intMsgCnt, 4) == "help")
	{
		mutexMsg.lock();
		SendMsgQueue.push(MsgType{ eve.fromDiscuss, strHlpMsg,DiscussMsg });
		mutexMsg.unlock();
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "st")
	{
		intMsgCnt += 2;
		while (isspace(strLowerMessage[intMsgCnt]))
			intMsgCnt++;
		if (intMsgCnt == strLowerMessage.length())
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromDiscuss,strStErr,DiscussMsg });
			mutexMsg.unlock();
			return;
		}
		if (strLowerMessage.substr(intMsgCnt, 3) == "clr")
		{
			if (CharacterProp.count(SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)))
			{
				CharacterProp.erase(SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss));
			}
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromDiscuss,strPropCleared,DiscussMsg });
			mutexMsg.unlock();
			return;
		}
		else if (strLowerMessage.substr(intMsgCnt, 3) == "del") {
			intMsgCnt += 3;
			while (isspace(strLowerMessage[intMsgCnt]))
				intMsgCnt++;
			string strSkillName;
			while (intMsgCnt != strLowerMessage.length() && !isspace(strLowerMessage[intMsgCnt]) && !(strLowerMessage[intMsgCnt] == '|'))
			{
				strSkillName += strLowerMessage[intMsgCnt];
				intMsgCnt++;
			}
			if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
			if (CharacterProp.count(SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)) && CharacterProp[SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)].count(strSkillName))
			{
				CharacterProp[SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)].erase(strSkillName);
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromDiscuss,strPropDeleted,DiscussMsg });
				mutexMsg.unlock();
			}
			else
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromDiscuss,strPropNotFound,DiscussMsg });
				mutexMsg.unlock();
			}
			return;
		}
		else if (strLowerMessage.substr(intMsgCnt, 4) == "show") {
			intMsgCnt += 4;
			while (isspace(strLowerMessage[intMsgCnt]))
				intMsgCnt++;
			string strSkillName;
			while (intMsgCnt != strLowerMessage.length() && !isspace(strLowerMessage[intMsgCnt]) && !(strLowerMessage[intMsgCnt] == '|'))
			{
				strSkillName += strLowerMessage[intMsgCnt];
				intMsgCnt++;
			}
			if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
			if (CharacterProp.count(SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)) && CharacterProp[SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)].count(strSkillName))
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromDiscuss,format(strProp,{ strNickName,strSkillName,to_string(CharacterProp[SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)][strSkillName]) }),DiscussMsg });
				mutexMsg.unlock();
			}
			else if (SkillDefaultVal.count(strSkillName))
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromDiscuss,format(strProp,{ strNickName,strSkillName,to_string(SkillDefaultVal[strSkillName]) }),DiscussMsg });
				mutexMsg.unlock();
			}
			else
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromDiscuss,strPropNotFound,DiscussMsg });
				mutexMsg.unlock();
			}
			return;
		}
		bool boolError = false;
		while (intMsgCnt != strLowerMessage.length())
		{
			string strSkillName;
			while (intMsgCnt != strLowerMessage.length() && !isdigit(strLowerMessage[intMsgCnt]) && !isspace(strLowerMessage[intMsgCnt]) && strLowerMessage[intMsgCnt] != '='&&strLowerMessage[intMsgCnt] != ':')
			{
				strSkillName += strLowerMessage[intMsgCnt];
				intMsgCnt++;
			}
			if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
			while (isspace(strLowerMessage[intMsgCnt]) || strLowerMessage[intMsgCnt] == '=' || strLowerMessage[intMsgCnt] == ':')intMsgCnt++;
			string strSkillVal;
			while (isdigit(strLowerMessage[intMsgCnt]))
			{
				strSkillVal += strLowerMessage[intMsgCnt];
				intMsgCnt++;
			}
			if (strSkillName.empty() || strSkillVal.empty() || strSkillVal.length() > 3)
			{
				boolError = true;
				break;
			}
			CharacterProp[SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)][strSkillName] = stoi(strSkillVal);
			while (isspace(strLowerMessage[intMsgCnt]) || strLowerMessage[intMsgCnt] == '|')intMsgCnt++;
		}
		if (boolError) {
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromDiscuss,strPropErr,DiscussMsg });
			mutexMsg.unlock();
		}
		else {

			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromDiscuss,strSetPropSuccess,DiscussMsg });
			mutexMsg.unlock();
		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "ri")
	{
		intMsgCnt += 2;
		while (isspace(strLowerMessage[intMsgCnt]))intMsgCnt++;
		string strinit = "D20";
		if (strLowerMessage[intMsgCnt] == '+' || strLowerMessage[intMsgCnt] == '-')
		{
			strinit += strLowerMessage[intMsgCnt];
			intMsgCnt++;
			while (isspace(strLowerMessage[intMsgCnt]))intMsgCnt++;
		}
		else if (isdigit(strLowerMessage[intMsgCnt]))
			strinit += '+';
		while (isdigit(strLowerMessage[intMsgCnt]))
		{
			strinit += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		string strname;
		while (isspace(strLowerMessage[intMsgCnt]))
		{
			intMsgCnt++;
		}
		strname = eve.message.substr(intMsgCnt);
		if (strname.empty())
			strname = strNickName;
		else
			strname = "[" + strname + "]";
		RD initdice(strinit);
		int intFirstTimeRes = initdice.Roll();
		if (intFirstTimeRes == Value_Err)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromDiscuss,strValueErr,DiscussMsg });
			mutexMsg.unlock();
			return;
		}
		else if (intFirstTimeRes == Input_Err)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromDiscuss,strInputErr,DiscussMsg });
			mutexMsg.unlock();
			return;
		}
		else if (intFirstTimeRes == ZeroDice_Err)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromDiscuss,strZeroDiceErr,DiscussMsg });
			mutexMsg.unlock();
			return;
		}
		else if (intFirstTimeRes == ZeroType_Err)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromDiscuss,strZeroTypeErr,DiscussMsg });
			mutexMsg.unlock();
			return;
		}
		else if (intFirstTimeRes == DiceTooBig_Err)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromDiscuss,strDiceTooBigErr,DiscussMsg });
			mutexMsg.unlock();
			return;
		}
		else if (intFirstTimeRes == TypeTooBig_Err)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromDiscuss,strTypeTooBigErr,DiscussMsg });
			mutexMsg.unlock();
			return;
		}
		else if (intFirstTimeRes == AddDiceVal_Err)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromDiscuss,strAddDiceValErr,DiscussMsg });
			mutexMsg.unlock();
			return;
		}
		else if (intFirstTimeRes != 0)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromDiscuss,strUnknownErr,DiscussMsg });
			mutexMsg.unlock();
			return;
		}
		ilInitListDiscuss.insert(eve.fromDiscuss, initdice.intTotal, strname);
		string strReply;
		strReply = strname + "的先攻骰点：" + strinit + '=' + to_string(initdice.intTotal);
		mutexMsg.lock();
		SendMsgQueue.push(MsgType{ eve.fromDiscuss,strReply,DiscussMsg });
		mutexMsg.unlock();
	}
	else if (strLowerMessage.substr(intMsgCnt, 4) == "init")
	{
		intMsgCnt += 4;
		string strReply;
		while (isspace(strLowerMessage[intMsgCnt]))intMsgCnt++;
		if (strLowerMessage.substr(intMsgCnt, 3) == "clr")
		{
			if (ilInitListDiscuss.clear(eve.fromDiscuss))
				strReply = "成功清除先攻记录！";
			else
				strReply = "列表为空！";
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromDiscuss,strReply,DiscussMsg });
			mutexMsg.unlock();
			return;
		}
		ilInitListDiscuss.show(eve.fromDiscuss, strReply);
		mutexMsg.lock();
		SendMsgQueue.push(MsgType{ eve.fromDiscuss,strReply,DiscussMsg });
		mutexMsg.unlock();
	}
	else if (strLowerMessage[intMsgCnt] == 'w')
	{
		intMsgCnt++;
		bool boolDetail = false;
		if (strLowerMessage[intMsgCnt] == 'w')
		{
			intMsgCnt++;
			boolDetail = true;
		}
		bool isHidden = false;
		if (strLowerMessage[intMsgCnt] == 'h')
		{
			isHidden = true;
			intMsgCnt += 1;
		}
		while (eve.message[intMsgCnt] == ' ')
			intMsgCnt++;

		string strMainDice;
		string strReason;
		int intTmpMsgCnt;
		for (intTmpMsgCnt = intMsgCnt; intTmpMsgCnt != eve.message.length() && eve.message[intTmpMsgCnt] != ' '; intTmpMsgCnt++)
		{
			if (!isdigit(strLowerMessage[intTmpMsgCnt]) && strLowerMessage[intTmpMsgCnt] != 'd' &&strLowerMessage[intTmpMsgCnt] != 'k'&&strLowerMessage[intTmpMsgCnt] != 'p'&&strLowerMessage[intTmpMsgCnt] != 'b'&&strLowerMessage[intTmpMsgCnt] != 'f'&& strLowerMessage[intTmpMsgCnt] != '+'&&strLowerMessage[intTmpMsgCnt] != '-' && strLowerMessage[intTmpMsgCnt] != '#'&& strLowerMessage[intTmpMsgCnt] != 'a')
			{
				break;
			}
		}
		strMainDice = strLowerMessage.substr(intMsgCnt, intTmpMsgCnt - intMsgCnt);
		while (eve.message[intTmpMsgCnt] == ' ')
			intTmpMsgCnt++;
		strReason = eve.message.substr(intTmpMsgCnt);


		int intTurnCnt = 1;
		if (strMainDice.find("#") != string::npos)
		{
			string strTurnCnt = strMainDice.substr(0, strMainDice.find("#"));
			if (strTurnCnt.empty())
				strTurnCnt = "1";
			strMainDice = strMainDice.substr(strMainDice.find("#") + 1);
			RD rdTurnCnt(strTurnCnt, eve.fromQQ);
			int intRdTurnCntRes = rdTurnCnt.Roll();
			if (intRdTurnCntRes == Value_Err)
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromDiscuss,strValueErr,DiscussMsg });
				mutexMsg.unlock();
				return;
			}
			else if (intRdTurnCntRes == Input_Err)
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromDiscuss,strInputErr,DiscussMsg });
				mutexMsg.unlock();
				return;
			}
			else if (intRdTurnCntRes == ZeroDice_Err)
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromDiscuss,strZeroDiceErr,DiscussMsg });
				mutexMsg.unlock();
				return;
			}
			else if (intRdTurnCntRes == ZeroType_Err)
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromDiscuss,strZeroTypeErr,DiscussMsg });
				mutexMsg.unlock();
				return;
			}
			else if (intRdTurnCntRes == DiceTooBig_Err)
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromDiscuss,strDiceTooBigErr,DiscussMsg });
				mutexMsg.unlock();
				return;
			}
			else if (intRdTurnCntRes == TypeTooBig_Err)
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromDiscuss,strTypeTooBigErr,DiscussMsg });
				mutexMsg.unlock();
				return;
			}
			else if (intRdTurnCntRes == AddDiceVal_Err)
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromDiscuss,strAddDiceValErr,DiscussMsg });
				mutexMsg.unlock();
				return;
			}
			else if (intRdTurnCntRes != 0)
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromDiscuss,strUnknownErr,DiscussMsg });
				mutexMsg.unlock();
				return;
			}
			if (rdTurnCnt.intTotal > 10)
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromDiscuss,strRollTimeExceeded,DiscussMsg });
				mutexMsg.unlock();
				return;
			}
			else if (rdTurnCnt.intTotal <= 0)
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromDiscuss,strRollTimeErr,DiscussMsg });
				mutexMsg.unlock();
				return;
			}
			intTurnCnt = rdTurnCnt.intTotal;
			if (strTurnCnt.find("d") != string::npos)
			{
				string strTurnNotice = strNickName + "的掷骰轮数: " + rdTurnCnt.FormShortString() + "轮";
				if (!isHidden)
				{
					mutexMsg.lock();
					SendMsgQueue.push(MsgType{ eve.fromDiscuss,strTurnNotice,DiscussMsg });
					mutexMsg.unlock();
				}
				else
				{
					strTurnNotice = "在多人聊天中 " + strTurnNotice;
					mutexMsg.lock();
					SendMsgQueue.push(MsgType{ eve.fromQQ,strTurnNotice,PrivateMsg });
					mutexMsg.unlock();
					auto range = ObserveDiscuss.equal_range(eve.fromDiscuss);
					for (auto it = range.first; it != range.second; it++)
					{
						if (it->second != eve.fromQQ)
						{
							mutexMsg.lock();
							SendMsgQueue.push(MsgType{ it->second,strTurnNotice,PrivateMsg });
							mutexMsg.unlock();
						}
					}
				}


			}
		}
		string strFirstDice = strMainDice.substr(0, strMainDice.find('+') < strMainDice.find('-') ? strMainDice.find('+') : strMainDice.find('-'));
		bool boolAdda10 = true;
		for (auto i : strFirstDice)
		{
			if (!isdigit(i))
			{
				boolAdda10 = false;
				break;
			}
		}
		if (boolAdda10)
			strMainDice.insert(strFirstDice.length(), "a10");
		RD rdMainDice(strMainDice, eve.fromQQ);

		int intFirstTimeRes = rdMainDice.Roll();
		if (intFirstTimeRes == Value_Err)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromDiscuss,strValueErr,DiscussMsg });
			mutexMsg.unlock();
			return;
		}
		else if (intFirstTimeRes == Input_Err)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromDiscuss,strInputErr,DiscussMsg });
			mutexMsg.unlock();
			return;
		}
		else if (intFirstTimeRes == ZeroDice_Err)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromDiscuss,strZeroDiceErr,DiscussMsg });
			mutexMsg.unlock();
			return;
		}
		else if (intFirstTimeRes == ZeroType_Err)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromDiscuss,strZeroTypeErr,DiscussMsg });
			mutexMsg.unlock();
			return;
		}
		else if (intFirstTimeRes == DiceTooBig_Err)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromDiscuss,strDiceTooBigErr,DiscussMsg });
			mutexMsg.unlock();
			return;
		}
		else if (intFirstTimeRes == TypeTooBig_Err)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromDiscuss,strTypeTooBigErr,DiscussMsg });
			mutexMsg.unlock();
			return;
		}
		else if (intFirstTimeRes == AddDiceVal_Err)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromDiscuss,strAddDiceValErr,DiscussMsg });
			mutexMsg.unlock();
			return;
		}
		else if (intFirstTimeRes != 0)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromDiscuss,strUnknownErr,DiscussMsg });
			mutexMsg.unlock();
			return;
		}
		if (!boolDetail && intTurnCnt != 1)
		{
			string strAns = strNickName + "骰出了: " + to_string(intTurnCnt) + "次" + rdMainDice.strDice + ": { ";
			if (!strReason.empty())
				strAns.insert(0, "由于" + strReason + " ");
			vector<int> vintExVal;
			while (intTurnCnt--)
			{
				rdMainDice.Roll();
				strAns += to_string(rdMainDice.intTotal);
				if (intTurnCnt != 0)
					strAns += ",";
				if ((rdMainDice.strDice == "D100" || rdMainDice.strDice == "1D100") && (rdMainDice.intTotal <= 5 || rdMainDice.intTotal >= 96))
					vintExVal.push_back(rdMainDice.intTotal);
			}
			strAns += " }";
			if (!vintExVal.empty())
			{
				strAns += ",极值: ";
				for (auto it = vintExVal.cbegin(); it != vintExVal.cend(); it++)
				{
					strAns += to_string(*it);
					if (it != vintExVal.cend() - 1)
						strAns += ",";
				}
			}
			if (!isHidden)
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromDiscuss,strAns,DiscussMsg });
				mutexMsg.unlock();
			}
			else
			{
				strAns = "在多人聊天中 " + strAns;
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromQQ,strAns,PrivateMsg });
				mutexMsg.unlock();
				auto range = ObserveDiscuss.equal_range(eve.fromDiscuss);
				for (auto it = range.first; it != range.second; it++)
				{
					if (it->second != eve.fromQQ)
					{
						mutexMsg.lock();
						SendMsgQueue.push(MsgType{ it->second,strAns,PrivateMsg });
						mutexMsg.unlock();
					}
				}
			}

		}
		else
		{
			while (intTurnCnt--)
			{
				rdMainDice.Roll();
				string strAns = strNickName + "骰出了: " + (boolDetail ? rdMainDice.FormCompleteString() : rdMainDice.FormShortString());
				if (!strReason.empty())
					strAns.insert(0, "由于" + strReason + " ");
				if (!isHidden)
				{
					mutexMsg.lock();
					SendMsgQueue.push(MsgType{ eve.fromDiscuss,strAns,DiscussMsg });
					mutexMsg.unlock();
				}
				else
				{
					strAns = "在多人聊天中 " + strAns;
					mutexMsg.lock();
					SendMsgQueue.push(MsgType{ eve.fromQQ,strAns,PrivateMsg });
					mutexMsg.unlock();
					auto range = ObserveDiscuss.equal_range(eve.fromDiscuss);
					for (auto it = range.first; it != range.second; it++)
					{
						if (it->second != eve.fromQQ)
						{
							mutexMsg.lock();
							SendMsgQueue.push(MsgType{ it->second,strAns,PrivateMsg });
							mutexMsg.unlock();
						}
					}
				}

			}
		}
		if (isHidden)
		{
			string strReply = strNickName + "进行了一次暗骰";
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromDiscuss,strReply,DiscussMsg });
			mutexMsg.unlock();
		}

	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "ob")
	{
		intMsgCnt += 2;
		while (strLowerMessage[intMsgCnt] == ' ')
			intMsgCnt++;
		string Command = strLowerMessage.substr(intMsgCnt, eve.message.find(' ', intMsgCnt) - intMsgCnt);
		if (Command == "on")
		{
			if (DisabledOBDiscuss.count(eve.fromDiscuss))
			{
				DisabledOBDiscuss.erase(eve.fromDiscuss);
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromDiscuss,  "成功在本多人聊天中启用旁观模式!",DiscussMsg });
				mutexMsg.unlock();
			}
			else
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromDiscuss,  "在本多人聊天中旁观模式没有被禁用!",DiscussMsg });
				mutexMsg.unlock();

			}
			return;
		}
		else if (Command == "off")
		{
			if (!DisabledOBDiscuss.count(eve.fromDiscuss))
			{
				DisabledOBDiscuss.insert(eve.fromDiscuss);
				ObserveDiscuss.clear();
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromDiscuss,  "成功在本多人聊天中禁用旁观模式!",DiscussMsg });
				mutexMsg.unlock();
			}
			else
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromDiscuss,  "在本多人聊天中旁观模式没有被启用!",DiscussMsg });
				mutexMsg.unlock();
			}
			return;
		}
		if (DisabledOBDiscuss.count(eve.fromDiscuss))
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromDiscuss,  "在本多人聊天中旁观模式已被禁用!",DiscussMsg });
			mutexMsg.unlock();
			return;
		}
		if (Command == "list")
		{
			string Msg = "当前的旁观者有:";
			auto Range = ObserveDiscuss.equal_range(eve.fromDiscuss);
			for (auto it = Range.first; it != Range.second; ++it)
			{
				pair<long long, long long> ObDiscuss;
				ObDiscuss.first = it->second;
				ObDiscuss.second = eve.fromDiscuss;
				Msg += "\n" + (DiscussName.count(ObDiscuss) ? DiscussName[ObDiscuss] : getStrangerInfo(it->second).nick) + "(" + to_string(it->second) + ")";
			}
			string strReply = Msg == "当前的旁观者有:" ? "当前暂无旁观者" : Msg;
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromDiscuss,  strReply,DiscussMsg });
			mutexMsg.unlock();
		}
		else if (Command == "clr")
		{
			ObserveDiscuss.erase(eve.fromDiscuss);
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromDiscuss,  "成功删除所有旁观者!",DiscussMsg });
			mutexMsg.unlock();
		}
		else if (Command == "exit")
		{
			auto Range = ObserveDiscuss.equal_range(eve.fromDiscuss);
			for (auto it = Range.first; it != Range.second; ++it)
			{
				if (it->second == eve.fromQQ)
				{
					ObserveDiscuss.erase(it);
					string strReply = strNickName + "成功退出旁观模式!";
					mutexMsg.lock();
					SendMsgQueue.push(MsgType{ eve.fromDiscuss,  strReply,DiscussMsg });
					mutexMsg.unlock();
					eve.message_block();
					return;
				}
			}
			string strReply = strNickName + "没有加入旁观模式!";
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromDiscuss,  strReply,DiscussMsg });
			mutexMsg.unlock();
		}
		else
		{
			auto Range = ObserveDiscuss.equal_range(eve.fromDiscuss);
			for (auto it = Range.first; it != Range.second; ++it)
			{
				if (it->second == eve.fromQQ)
				{
					string strReply = strNickName + "已经处于旁观模式!";
					mutexMsg.lock();
					SendMsgQueue.push(MsgType{ eve.fromDiscuss,  strReply,DiscussMsg });
					mutexMsg.unlock();
					eve.message_block();
					return;
				}
			}
			ObserveDiscuss.insert(make_pair(eve.fromDiscuss, eve.fromQQ));
			string strReply = strNickName + "成功加入旁观模式!";
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromDiscuss,  strReply,DiscussMsg });
			mutexMsg.unlock();
		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "ti")
	{
		string strAns = strNickName + "的疯狂发作-临时症状:\n";
		TempInsane(strAns);
		mutexMsg.lock();
		SendMsgQueue.push(MsgType{ eve.fromDiscuss,  strAns,DiscussMsg });
		mutexMsg.unlock();
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "li")
	{
		string strAns = strNickName + "的疯狂发作-总结症状:\n";
		LongInsane(strAns);
		mutexMsg.lock();
		SendMsgQueue.push(MsgType{ eve.fromDiscuss,  strAns,DiscussMsg });
		mutexMsg.unlock();
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "sc")
	{
		intMsgCnt += 2;
		while (strLowerMessage[intMsgCnt] == ' ')
			intMsgCnt++;
		string SanCost = strLowerMessage.substr(intMsgCnt, eve.message.find(' ', intMsgCnt) - intMsgCnt);
		intMsgCnt += SanCost.length();
		while (strLowerMessage[intMsgCnt] == ' ')
			intMsgCnt++;
		string San;
		while (isdigit(strLowerMessage[intMsgCnt])) {
			San += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (SanCost.empty() || SanCost.find("/") == string::npos)
		{

			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromDiscuss,strSCInvalid,DiscussMsg });
			mutexMsg.unlock();

			return;
		}
		if (San.empty() && !(CharacterProp.count(SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)) && CharacterProp[SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)].count("理智")))
		{

			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromDiscuss,strSanInvalid,DiscussMsg });
			mutexMsg.unlock();

			return;
		}
		RD rdTest1(SanCost.substr(0, SanCost.find("/")));
		RD rdTest2(SanCost.substr(SanCost.find("/") + 1));
		if (rdTest1.Roll() != 0 || rdTest2.Roll() != 0)
		{

			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromDiscuss,strSCInvalid,DiscussMsg });
			mutexMsg.unlock();

			return;
		}
		if (San.length() >= 3)
		{

			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromDiscuss,strSanInvalid,DiscussMsg });
			mutexMsg.unlock();

			return;
		}
		int intSan = San.empty() ? CharacterProp[SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)]["理智"] : stoi(San);
		if (intSan == 0)
		{

			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromDiscuss,strSanInvalid,DiscussMsg });
			mutexMsg.unlock();

			return;
		}
		string strAns = strNickName + "的Sancheck:\n1D100=";
		int intTmpRollRes = Randint(1, 100);
		strAns += to_string(intTmpRollRes);

		if (intTmpRollRes <= intSan)
		{
			strAns += " 成功\n你的San值减少" + SanCost.substr(0, SanCost.find("/"));
			RD rdSan(SanCost.substr(0, SanCost.find("/")));
			rdSan.Roll();
			if (SanCost.substr(0, SanCost.find("/")).find("d") != string::npos)
				strAns += "=" + to_string(rdSan.intTotal);
			strAns += +"点,当前剩余" + to_string(max(0, intSan - rdSan.intTotal)) + "点";
			if (San.empty())
			{
				CharacterProp[SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)]["理智"] = max(0, intSan - rdSan.intTotal);
			}
		}
		else
		{
			strAns += " 失败\n你的San值减少" + SanCost.substr(SanCost.find("/") + 1);
			RD rdSan(SanCost.substr(SanCost.find("/") + 1));
			rdSan.Roll();
			if (SanCost.substr(SanCost.find("/") + 1).find("d") != string::npos)
				strAns += "=" + to_string(rdSan.intTotal);
			strAns += +"点,当前剩余" + to_string(max(0, intSan - rdSan.intTotal)) + "点";
			if (San.empty())
			{
				CharacterProp[SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)]["理智"] = max(0, intSan - rdSan.intTotal);
			}
		}
		mutexMsg.lock();
		SendMsgQueue.push(MsgType{ eve.fromDiscuss,strAns,DiscussMsg });
		mutexMsg.unlock();
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "en")
	{
		intMsgCnt += 2;
		while (strLowerMessage[intMsgCnt] == ' ')
			intMsgCnt++;
		string strSkillName;
		while (intMsgCnt != eve.message.length() && !isdigit(eve.message[intMsgCnt]) && !isspace(eve.message[intMsgCnt]))
		{
			strSkillName += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
		while (isspace(eve.message[intMsgCnt]))
			intMsgCnt++;
		string strCurrentValue;
		while (isdigit(eve.message[intMsgCnt]))
		{
			strCurrentValue += eve.message[intMsgCnt];
			intMsgCnt++;
		}
		int intCurrentVal;
		if (strCurrentValue.empty())
		{
			if (CharacterProp.count(SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)) && CharacterProp[SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)].count(strSkillName))
			{
				intCurrentVal = CharacterProp[SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)][strSkillName];
			}
			else if (SkillDefaultVal.count(strSkillName))
			{
				intCurrentVal = SkillDefaultVal[strSkillName];
			}
			else
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromDiscuss,strEnValInvalid,DiscussMsg });
				mutexMsg.unlock();
				return;
			}

		}
		else
		{
			if (strCurrentValue.length() > 3)
			{

				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromDiscuss,strEnValInvalid,DiscussMsg });
				mutexMsg.unlock();

				return;
			}
			intCurrentVal = stoi(strCurrentValue);
		}
		string strAns = strNickName + "的" + strSkillName + "增强或成长检定:\n1D100=";
		int intTmpRollRes = Randint(1, 100);
		strAns += to_string(intTmpRollRes) + "/" + to_string(intCurrentVal);

		if (intTmpRollRes <= intCurrentVal && intTmpRollRes <= 95)
		{
			strAns += " 失败!\n你的" + (strSkillName.empty() ? "属性或技能值" : strSkillName) + "没有变化!";
		}
		else
		{
			strAns += " 成功!\n你的" + (strSkillName.empty() ? "属性或技能值" : strSkillName) + "增加1D10=";
			int intTmpRollD10 = Randint(1, 10);
			strAns += to_string(intTmpRollD10) + "点,当前为" + to_string(intCurrentVal + intTmpRollD10) + "点";
			if (strCurrentValue.empty())
			{
				CharacterProp[SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)][strSkillName] = intCurrentVal + intTmpRollD10;
			}
		}
		mutexMsg.lock();
		SendMsgQueue.push(MsgType{ eve.fromDiscuss,strAns,DiscussMsg });
		mutexMsg.unlock();
	}
	else if (strLowerMessage.substr(intMsgCnt, 4) == "jrrp")
	{
		intMsgCnt += 4;
		while (strLowerMessage[intMsgCnt] == ' ')
			intMsgCnt++;
		string Command = strLowerMessage.substr(intMsgCnt, eve.message.find(' ', intMsgCnt) - intMsgCnt);
		if (Command == "on")
		{
			if (DisabledJRRPDiscuss.count(eve.fromDiscuss))
			{
				DisabledJRRPDiscuss.erase(eve.fromDiscuss);
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromDiscuss,"成功在此多人聊天中启用JRRP!",DiscussMsg });
				mutexMsg.unlock();
			}
			else
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromDiscuss,"在此多人聊天中JRRP没有被禁用!",DiscussMsg });
				mutexMsg.unlock();
			}
			return;
		}
		else if (Command == "off")
		{
			if (!DisabledJRRPDiscuss.count(eve.fromDiscuss))
			{
				DisabledJRRPDiscuss.insert(eve.fromDiscuss);
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromDiscuss,"成功在此多人聊天中禁用JRRP!",DiscussMsg });
				mutexMsg.unlock();
			}
			else
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromDiscuss,"在此多人聊天中JRRP没有被启用!",DiscussMsg });
				mutexMsg.unlock();
			}
			return;
		}
		if (DisabledJRRPDiscuss.count(eve.fromDiscuss))
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromDiscuss,"在此多人聊天中JRRP已被禁用!",DiscussMsg });
			mutexMsg.unlock();
			return;
		}
		char cstrDate[100] = {};
		time_t time_tTime = 0;
		time(&time_tTime);
		tm tmTime;
		localtime_s(&tmTime, &time_tTime);
		strftime(cstrDate, 100, "%F", &tmTime);
		if (JRRP.count(eve.fromQQ) && JRRP[eve.fromQQ].Date == cstrDate)
		{
			string strReply = strNickName + "今天的人品值是:" + to_string(JRRP[eve.fromQQ].RPVal);
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromDiscuss,strReply,DiscussMsg });
			mutexMsg.unlock();
		}
		else
		{
			normal_distribution<double> NormalDistribution(60, 15);
			mt19937 Generator(static_cast<unsigned int> (GetCycleCount()));
			int JRRPRes;
			do
			{
				JRRPRes = static_cast<int> (NormalDistribution(Generator));
			} while (JRRPRes <= 0 || JRRPRes > 100);
			JRRP[eve.fromQQ].Date = cstrDate;
			JRRP[eve.fromQQ].RPVal = JRRPRes;
			string strReply(strNickName + "今天的人品值是:" + to_string(JRRP[eve.fromQQ].RPVal));
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromDiscuss,strReply,DiscussMsg });
			mutexMsg.unlock();
		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "nn")
	{
		intMsgCnt += 2;
		while (eve.message[intMsgCnt] == ' ')
			intMsgCnt++;
		string name = eve.message.substr(intMsgCnt);
		if (name.length() > 50)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromDiscuss, strNameTooLongErr, DiscussMsg });
			mutexMsg.unlock();
			return;
		}
		if (!name.empty())
		{
			DiscussName[pairQQDiscuss] = name;
			name = "[" + name + "]";
			string strReply = "已将" + strNickName + "的名称更改为" + name;
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromDiscuss,strReply,DiscussMsg });
			mutexMsg.unlock();
		}
		else
		{
			if (DiscussName.count(pairQQDiscuss))
			{
				DiscussName.erase(pairQQDiscuss);
				string strReply = "已将" + strNickName + "的名称删除";
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromDiscuss,strReply,DiscussMsg });
				mutexMsg.unlock();
			}
			else
			{
				string strReply = strNickName + strNameDelErr;
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromDiscuss,strReply,DiscussMsg });
				mutexMsg.unlock();
			}
		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 5) == "rules")
	{
		intMsgCnt += 5;
		while (eve.message[intMsgCnt] == ' ')
			intMsgCnt++;
		string strSearch = eve.message.substr(intMsgCnt);
		for (auto &n : strSearch)
			n = toupper(n);
		for (int i = 0; i != strSearch.length(); ++i)
			if (strSearch[i] == ' ')
			{
				strSearch.erase(strSearch.begin() + i);
				i--;
			}
		strSearch = "||" + strSearch + "||";
		int Loc = strRules.find(strSearch);
		if (Loc != string::npos || strSearch == "||战斗||" || strSearch == "||战斗流程||" || strSearch == "||战斗伤害||")
		{
			if (strSearch == "||战斗||" || strSearch == "||战斗流程||")
			{
				string IMAGE = "[CQ:image,file=";
				string IMAGENAME = "Combat.jpg";
				IMAGE += msg_encode(IMAGENAME, true) + "]";
				int res = sendDiscussMsg(eve.fromDiscuss, IMAGE);
				if (res < 0)
				{
					mutexMsg.lock();
					SendMsgQueue.push(MsgType{ eve.fromDiscuss,"未找到对应的规则信息!",DiscussMsg });
					mutexMsg.unlock();
				}
			}
			else if (strSearch == "||战斗伤害||")
			{
				string IMAGE = "[CQ:image,file=";
				string IMAGENAME = "Combat1.jpg";
				IMAGE += msg_encode(IMAGENAME, true) + "]";
				int res = sendDiscussMsg(eve.fromDiscuss, IMAGE);
				if (res < 0)
				{
					mutexMsg.lock();
					SendMsgQueue.push(MsgType{ eve.fromDiscuss,"未找到对应的规则信息!",DiscussMsg });
					mutexMsg.unlock();
				}
			}
			else
			{
				int LocStart = strRules.find("]", Loc) + 1;
				string strReply = strRules.substr(LocStart, strRules.find("[", LocStart) - LocStart - 1);
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromDiscuss,strReply,DiscussMsg });
				mutexMsg.unlock();
			}
		}
		else
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromDiscuss,"未找到对应的规则信息!",DiscussMsg });
			mutexMsg.unlock();
		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "me")
	{
		intMsgCnt += 2;
		while (strLowerMessage[intMsgCnt] == ' ')
			intMsgCnt++;
		string strAction = strLowerMessage.substr(intMsgCnt);
		if (strAction == "on")
		{
			if (DisabledMEDiscuss.count(eve.fromDiscuss))
			{
				DisabledMEDiscuss.erase(eve.fromDiscuss);
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromDiscuss,"成功在本多人聊天中启用.me命令!",DiscussMsg });
				mutexMsg.unlock();
			}
			else
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromDiscuss,"在本多人聊天中.me命令没有被禁用!",DiscussMsg });
				mutexMsg.unlock();
			}
			return;
		}
		else if (strAction == "off")
		{
			if (!DisabledMEDiscuss.count(eve.fromDiscuss))
			{
				DisabledMEDiscuss.insert(eve.fromDiscuss);
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromDiscuss,"成功在本多人聊天中禁用.me命令!",DiscussMsg });
				mutexMsg.unlock();
			}
			else
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromDiscuss,"在本多人聊天中.me命令没有被启用!",DiscussMsg });
				mutexMsg.unlock();
			}
			return;
		}
		if (DisabledMEDiscuss.count(eve.fromDiscuss))
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromDiscuss,"在本多人聊天中.me命令已被禁用!",DiscussMsg });
			mutexMsg.unlock();
			return;
		}
		if (strAction.empty())
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromDiscuss,"动作不能为空!",DiscussMsg });
			mutexMsg.unlock();
			return;
		}
		if (DisabledMEDiscuss.count(eve.fromDiscuss))
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromDiscuss,strMEDisabledErr,DiscussMsg });
			mutexMsg.unlock();
			return;
		}
		string strReply = strNickName + eve.message.substr(intMsgCnt);
		mutexMsg.lock();
		SendMsgQueue.push(MsgType{ eve.fromDiscuss,strReply,DiscussMsg });
		mutexMsg.unlock();
	}
	else if (strLowerMessage.substr(intMsgCnt, 3) == "set")
	{
		intMsgCnt += 3;
		while (strLowerMessage[intMsgCnt] == ' ')
			intMsgCnt++;
		string strDefaultDice = strLowerMessage.substr(intMsgCnt, strLowerMessage.find(" ", intMsgCnt) - intMsgCnt);
		while (strDefaultDice[0] == '0')
			strDefaultDice.erase(strDefaultDice.begin());
		if (strDefaultDice.empty())
			strDefaultDice = "100";
		for (auto charNumElement : strDefaultDice)
			if (!isdigit(charNumElement))
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromDiscuss,strSetInvalid,DiscussMsg });
				mutexMsg.unlock();
				return;
			}
		if (strDefaultDice.length() > 5)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromDiscuss,strSetTooBig,DiscussMsg });
			mutexMsg.unlock();
			return;
		}
		int intDefaultDice = stoi(strDefaultDice);
		if (intDefaultDice == 100)
			DefaultDice.erase(eve.fromQQ);
		else
			DefaultDice[eve.fromQQ] = intDefaultDice;
		string strSetSuccessReply = "已将" + strNickName + "的默认骰类型更改为D" + strDefaultDice;
		mutexMsg.lock();
		SendMsgQueue.push(MsgType{ eve.fromDiscuss,strSetSuccessReply,DiscussMsg });
		mutexMsg.unlock();
	}
	else if (strLowerMessage.substr(intMsgCnt, 5) == "coc6d")
	{
		string strReply = strNickName;
		COC6D(strReply);
		mutexMsg.lock();
		SendMsgQueue.push(MsgType{ eve.fromDiscuss,strReply,DiscussMsg });
		mutexMsg.unlock();
	}
	else if (strLowerMessage.substr(intMsgCnt, 4) == "coc6")
	{
		intMsgCnt += 4;
		if (strLowerMessage[intMsgCnt] == 's')
			intMsgCnt++;
		while (strLowerMessage[intMsgCnt] == ' ')
			intMsgCnt++;
		string strNum;
		while (isdigit(strLowerMessage[intMsgCnt]))
		{
			strNum += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (strNum.length() > 2)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromQQ,strCharacterTooBig,PrivateMsg });
			mutexMsg.unlock();
			return;
		}
		int intNum = stoi(strNum.empty() ? "1" : strNum);
		if (intNum > 10)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromDiscuss,strCharacterTooBig,DiscussMsg });
			mutexMsg.unlock();
			return;
		}
		if (intNum == 0)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromDiscuss,strCharacterCannotBeZero,DiscussMsg });
			mutexMsg.unlock();
			return;
		}
		string strReply = strNickName;
		COC6(strReply, intNum);
		mutexMsg.lock();
		SendMsgQueue.push(MsgType{ eve.fromDiscuss,strReply,DiscussMsg });
		mutexMsg.unlock();
	}
	else if (strLowerMessage.substr(intMsgCnt, 3) == "dnd")
	{
		intMsgCnt += 3;
		while (strLowerMessage[intMsgCnt] == ' ')
			intMsgCnt++;
		string strNum;
		while (isdigit(strLowerMessage[intMsgCnt]))
		{
			strNum += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (strNum.length() > 2)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromQQ,strCharacterTooBig,PrivateMsg });
			mutexMsg.unlock();
			return;
		}
		int intNum = stoi(strNum.empty() ? "1" : strNum);
		if (intNum > 10)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromDiscuss,strCharacterTooBig,DiscussMsg });
			mutexMsg.unlock();
			return;
		}
		if (intNum == 0)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromDiscuss,strCharacterCannotBeZero,DiscussMsg });
			mutexMsg.unlock();
			return;
		}
		string strReply = strNickName;
		DND(strReply, intNum);
		mutexMsg.lock();
		SendMsgQueue.push(MsgType{ eve.fromDiscuss,strReply,DiscussMsg });
		mutexMsg.unlock();
	}
	else if (strLowerMessage.substr(intMsgCnt, 5) == "coc7d" || strLowerMessage.substr(intMsgCnt, 4) == "cocd")
	{
		string strReply = strNickName;
		COC7D(strReply);
		mutexMsg.lock();
		SendMsgQueue.push(MsgType{ eve.fromDiscuss,strReply,DiscussMsg });
		mutexMsg.unlock();
	}
	else if (strLowerMessage.substr(intMsgCnt, 3) == "coc")
	{
		intMsgCnt += 3;
		if (strLowerMessage[intMsgCnt] == '7')
			intMsgCnt++;
		if (strLowerMessage[intMsgCnt] == 's')
			intMsgCnt++;
		while (strLowerMessage[intMsgCnt] == ' ')
			intMsgCnt++;
		string strNum;
		while (isdigit(strLowerMessage[intMsgCnt]))
		{
			strNum += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (strNum.length() > 2)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromQQ,strCharacterTooBig,PrivateMsg });
			mutexMsg.unlock();
			return;
		}
		int intNum = stoi(strNum.empty() ? "1" : strNum);
		if (intNum > 10)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromDiscuss,strCharacterTooBig,DiscussMsg });
			mutexMsg.unlock();
			return;
		}
		if (intNum == 0)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromDiscuss,strCharacterCannotBeZero,DiscussMsg });
			mutexMsg.unlock();
			return;
		}
		string strReply = strNickName;
		COC7(strReply, intNum);
		mutexMsg.lock();
		SendMsgQueue.push(MsgType{ eve.fromDiscuss,strReply,DiscussMsg });
		mutexMsg.unlock();
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "ra")
	{
		intMsgCnt += 2;
		RD rdD100("D100");
		string strSkillName;
		while (isspace(strLowerMessage[intMsgCnt]))intMsgCnt++;
		while (intMsgCnt != strLowerMessage.length() && !isdigit(strLowerMessage[intMsgCnt]) && !isspace(strLowerMessage[intMsgCnt]) && strLowerMessage[intMsgCnt] != '='&&strLowerMessage[intMsgCnt] != ':')
		{
			strSkillName += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
		while (isspace(strLowerMessage[intMsgCnt]) || strLowerMessage[intMsgCnt] == '=' || strLowerMessage[intMsgCnt] == ':')intMsgCnt++;
		string strSkillVal;
		while (isdigit(strLowerMessage[intMsgCnt]))
		{
			strSkillVal += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		string strReason;
		while (isspace(strLowerMessage[intMsgCnt]))
		{
			intMsgCnt++;
		}
		strReason = eve.message.substr(intMsgCnt);
		int intSkillVal = 0;
		if (strSkillVal.empty())
		{
			if (CharacterProp.count(SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)) && CharacterProp[SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)].count(strSkillName))
			{
				intSkillVal = CharacterProp[SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)][strSkillName];
			}
			else if (SkillDefaultVal.count(strSkillName))
			{
				intSkillVal = SkillDefaultVal[strSkillName];
			}
			else
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromDiscuss,strUnknownPropErr,DiscussMsg });
				mutexMsg.unlock();
				return;
			}
		}
		else if (strSkillVal.length() > 3)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromDiscuss,strPropErr,DiscussMsg });
			mutexMsg.unlock();
			return;
		}
		else {
			intSkillVal = stoi(strSkillVal);
		}
		rdD100.Roll();
		string strReply = strNickName + "进行" + strSkillName + "检定: " + rdD100.FormShortString() + "/" + to_string(intSkillVal) + " ";
		if (rdD100.intTotal <= 5)strReply += "大成功";
		else if (rdD100.intTotal <= intSkillVal / 5)strReply += "极难成功";
		else if (rdD100.intTotal <= intSkillVal / 2)strReply += "困难成功";
		else if (rdD100.intTotal <= intSkillVal)strReply += "成功";
		else if (rdD100.intTotal <= 95)strReply += "失败";
		else strReply += "大失败";
		if (!strReason.empty())
		{
			strReply = "由于" + strReason + " " + strReply;
		}
		mutexMsg.lock();
		SendMsgQueue.push(MsgType{ eve.fromDiscuss,strReply,DiscussMsg });
		mutexMsg.unlock();
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "rc")
	{
		intMsgCnt += 2;
		RD rdD100("D100");
		string strSkillName;
		while (isspace(strLowerMessage[intMsgCnt]))intMsgCnt++;
		while (intMsgCnt != strLowerMessage.length() && !isdigit(strLowerMessage[intMsgCnt]) && !isspace(strLowerMessage[intMsgCnt]) && strLowerMessage[intMsgCnt] != '='&&strLowerMessage[intMsgCnt] != ':')
		{
			strSkillName += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];
		while (isspace(strLowerMessage[intMsgCnt]) || strLowerMessage[intMsgCnt] == '=' || strLowerMessage[intMsgCnt] == ':')intMsgCnt++;
		string strSkillVal;
		while (isdigit(strLowerMessage[intMsgCnt]))
		{
			strSkillVal += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		string strReason;
		while (isspace(strLowerMessage[intMsgCnt]))
		{
			intMsgCnt++;
		}
		strReason = eve.message.substr(intMsgCnt);
		int intSkillVal = 0;
		if (strSkillVal.empty())
		{
			if (CharacterProp.count(SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)) && CharacterProp[SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)].count(strSkillName))
			{
				intSkillVal = CharacterProp[SourceType(eve.fromQQ, DiscussT, eve.fromDiscuss)][strSkillName];
			}
			else if (SkillDefaultVal.count(strSkillName))
			{
				intSkillVal = SkillDefaultVal[strSkillName];
			}
			else
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromDiscuss,strUnknownPropErr,DiscussMsg });
				mutexMsg.unlock();
				return;
			}
		}
		else if (strSkillVal.length() > 3)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromDiscuss,strPropErr,DiscussMsg });
			mutexMsg.unlock();
			return;
		}
		else {
			intSkillVal = stoi(strSkillVal);
		}
		rdD100.Roll();
		string strReply = strNickName + "进行" + strSkillName + "检定: " + rdD100.FormShortString() + "/" + to_string(intSkillVal) + " ";
		if (rdD100.intTotal == 1)strReply += "大成功";
		else if (rdD100.intTotal <= intSkillVal / 5)strReply += "极难成功";
		else if (rdD100.intTotal <= intSkillVal / 2)strReply += "困难成功";
		else if (rdD100.intTotal <= intSkillVal)strReply += "成功";
		else if (rdD100.intTotal <= 95 || (intSkillVal > 50 && rdD100.intTotal != 100))strReply += "失败";
		else strReply += "大失败";
		if (!strReason.empty())
		{
			strReply = "由于" + strReason + " " + strReply;
		}
		mutexMsg.lock();
		SendMsgQueue.push(MsgType{ eve.fromDiscuss,strReply,DiscussMsg });
		mutexMsg.unlock();
	}
	else if (strLowerMessage[intMsgCnt] == 'r' || strLowerMessage[intMsgCnt] == 'o' || strLowerMessage[intMsgCnt] == 'h' || strLowerMessage[intMsgCnt] == 'd')
	{
		bool isHidden = false;
		if (strLowerMessage[intMsgCnt] == 'h')
			isHidden = true;
		intMsgCnt += 1;
		bool boolDetail = true;
		if (eve.message[intMsgCnt] == 's')
		{
			boolDetail = false;
			intMsgCnt++;
		}
		if (strLowerMessage[intMsgCnt] == 'h')
		{
			isHidden = true;
			intMsgCnt += 1;
		}
		while (eve.message[intMsgCnt] == ' ')
			intMsgCnt++;
		string strMainDice;
		string strReason;
		bool tmpContainD = false;
		int intTmpMsgCnt;
		for (intTmpMsgCnt = intMsgCnt; intTmpMsgCnt != eve.message.length() && eve.message[intTmpMsgCnt] != ' '; intTmpMsgCnt++)
		{
			if (strLowerMessage[intTmpMsgCnt] == 'd' || strLowerMessage[intTmpMsgCnt] == 'p' || strLowerMessage[intTmpMsgCnt] == 'b' || strLowerMessage[intTmpMsgCnt] == '#' || strLowerMessage[intTmpMsgCnt] == 'f' || strLowerMessage[intTmpMsgCnt] == 'a')
				tmpContainD = true;
			if (!isdigit(strLowerMessage[intTmpMsgCnt]) && strLowerMessage[intTmpMsgCnt] != 'd' &&strLowerMessage[intTmpMsgCnt] != 'k'&&strLowerMessage[intTmpMsgCnt] != 'p'&&strLowerMessage[intTmpMsgCnt] != 'b'&&strLowerMessage[intTmpMsgCnt] != 'f'&& strLowerMessage[intTmpMsgCnt] != '+'&&strLowerMessage[intTmpMsgCnt] != '-' && strLowerMessage[intTmpMsgCnt] != '#' && strLowerMessage[intTmpMsgCnt] != 'a')
			{
				break;
			}
		}
		if (tmpContainD)
		{
			strMainDice = strLowerMessage.substr(intMsgCnt, intTmpMsgCnt - intMsgCnt);
			while (eve.message[intTmpMsgCnt] == ' ')
				intTmpMsgCnt++;
			strReason = eve.message.substr(intTmpMsgCnt);
		}
		else
			strReason = eve.message.substr(intMsgCnt);

		int intTurnCnt = 1;
		if (strMainDice.find("#") != string::npos)
		{
			string strTurnCnt = strMainDice.substr(0, strMainDice.find("#"));
			if (strTurnCnt.empty())
				strTurnCnt = "1";
			strMainDice = strMainDice.substr(strMainDice.find("#") + 1);
			RD rdTurnCnt(strTurnCnt, eve.fromQQ);
			int intRdTurnCntRes = rdTurnCnt.Roll();
			if (intRdTurnCntRes == Value_Err)
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromDiscuss,strValueErr,DiscussMsg });
				mutexMsg.unlock();
				return;
			}
			else if (intRdTurnCntRes == Input_Err)
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromDiscuss,strInputErr,DiscussMsg });
				mutexMsg.unlock();
				return;
			}
			else if (intRdTurnCntRes == ZeroDice_Err)
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromDiscuss,strZeroDiceErr,DiscussMsg });
				mutexMsg.unlock();
				return;
			}
			else if (intRdTurnCntRes == ZeroType_Err)
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromDiscuss,strZeroTypeErr,DiscussMsg });
				mutexMsg.unlock();
				return;
			}
			else if (intRdTurnCntRes == DiceTooBig_Err)
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromDiscuss,strDiceTooBigErr,DiscussMsg });
				mutexMsg.unlock();
				return;
			}
			else if (intRdTurnCntRes == TypeTooBig_Err)
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromDiscuss,strTypeTooBigErr,DiscussMsg });
				mutexMsg.unlock();
				return;
			}
			else if (intRdTurnCntRes == AddDiceVal_Err)
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromDiscuss,strAddDiceValErr,DiscussMsg });
				mutexMsg.unlock();
				return;
			}
			else if (intRdTurnCntRes != 0)
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromDiscuss,strUnknownErr,DiscussMsg });
				mutexMsg.unlock();
				return;
			}
			if (rdTurnCnt.intTotal > 10)
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromDiscuss,strRollTimeExceeded,DiscussMsg });
				mutexMsg.unlock();
				return;
			}
			else if (rdTurnCnt.intTotal <= 0)
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromDiscuss,strRollTimeErr,DiscussMsg });
				mutexMsg.unlock();
				return;
			}
			intTurnCnt = rdTurnCnt.intTotal;
			if (strTurnCnt.find("d") != string::npos)
			{
				string strTurnNotice = strNickName + "的掷骰轮数: " + rdTurnCnt.FormShortString() + "轮";
				if (!isHidden)
				{
					mutexMsg.lock();
					SendMsgQueue.push(MsgType{ eve.fromDiscuss,strTurnNotice,DiscussMsg });
					mutexMsg.unlock();
				}
				else
				{
					strTurnNotice = "在多人聊天中 " + strTurnNotice;
					mutexMsg.lock();
					SendMsgQueue.push(MsgType{ eve.fromQQ,strTurnNotice,PrivateMsg });
					mutexMsg.unlock();
					auto range = ObserveDiscuss.equal_range(eve.fromDiscuss);
					for (auto it = range.first; it != range.second; it++)
					{
						if (it->second != eve.fromQQ)
						{
							mutexMsg.lock();
							SendMsgQueue.push(MsgType{ it->second,strTurnNotice,PrivateMsg });
							mutexMsg.unlock();
						}
					}
				}


			}
		}
		RD rdMainDice(strMainDice, eve.fromQQ);

		int intFirstTimeRes = rdMainDice.Roll();
		if (intFirstTimeRes == Value_Err)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromDiscuss,strValueErr,DiscussMsg });
			mutexMsg.unlock();
			return;
		}
		else if (intFirstTimeRes == Input_Err)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromDiscuss,strInputErr,DiscussMsg });
			mutexMsg.unlock();
			return;
		}
		else if (intFirstTimeRes == ZeroDice_Err)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromDiscuss,strZeroDiceErr,DiscussMsg });
			mutexMsg.unlock();
			return;
		}
		else if (intFirstTimeRes == ZeroType_Err)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromDiscuss,strZeroTypeErr,DiscussMsg });
			mutexMsg.unlock();
			return;
		}
		else if (intFirstTimeRes == DiceTooBig_Err)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromDiscuss,strDiceTooBigErr,DiscussMsg });
			mutexMsg.unlock();
			return;
		}
		else if (intFirstTimeRes == TypeTooBig_Err)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromDiscuss,strTypeTooBigErr,DiscussMsg });
			mutexMsg.unlock();
			return;
		}
		else if (intFirstTimeRes == AddDiceVal_Err)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromDiscuss,strAddDiceValErr,DiscussMsg });
			mutexMsg.unlock();
			return;
		}
		else if (intFirstTimeRes != 0)
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromDiscuss,strUnknownErr,DiscussMsg });
			mutexMsg.unlock();
			return;
		}
		if (!boolDetail&&intTurnCnt != 1)
		{
			string strAns = strNickName + "骰出了: " + to_string(intTurnCnt) + "次" + rdMainDice.strDice + ": { ";
			if (!strReason.empty())
				strAns.insert(0, "由于" + strReason + " ");
			vector<int> vintExVal;
			while (intTurnCnt--)
			{
				rdMainDice.Roll();
				strAns += to_string(rdMainDice.intTotal);
				if (intTurnCnt != 0)
					strAns += ",";
				if ((rdMainDice.strDice == "D100" || rdMainDice.strDice == "1D100") && (rdMainDice.intTotal <= 5 || rdMainDice.intTotal >= 96))
					vintExVal.push_back(rdMainDice.intTotal);
			}
			strAns += " }";
			if (!vintExVal.empty())
			{
				strAns += ",极值: ";
				for (auto it = vintExVal.cbegin(); it != vintExVal.cend(); it++)
				{
					strAns += to_string(*it);
					if (it != vintExVal.cend() - 1)
						strAns += ",";
				}
			}
			if (!isHidden)
			{
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromDiscuss,strAns,DiscussMsg });
				mutexMsg.unlock();
			}
			else
			{
				strAns = "在多人聊天中 " + strAns;
				mutexMsg.lock();
				SendMsgQueue.push(MsgType{ eve.fromQQ,strAns,PrivateMsg });
				mutexMsg.unlock();
				auto range = ObserveDiscuss.equal_range(eve.fromDiscuss);
				for (auto it = range.first; it != range.second; it++)
				{
					if (it->second != eve.fromQQ)
					{
						mutexMsg.lock();
						SendMsgQueue.push(MsgType{ it->second,strAns,PrivateMsg });
						mutexMsg.unlock();
					}
				}
			}

		}
		else
		{
			while (intTurnCnt--)
			{
				rdMainDice.Roll();
				string strAns = strNickName + "骰出了: " + (boolDetail ? rdMainDice.FormCompleteString() : rdMainDice.FormShortString());
				if (!strReason.empty())
					strAns.insert(0, "由于" + strReason + " ");
				if (!isHidden)
				{
					mutexMsg.lock();
					SendMsgQueue.push(MsgType{ eve.fromDiscuss,strAns,DiscussMsg });
					mutexMsg.unlock();
				}
				else
				{
					strAns = "在多人聊天中 " + strAns;
					mutexMsg.lock();
					SendMsgQueue.push(MsgType{ eve.fromQQ,strAns,PrivateMsg });
					mutexMsg.unlock();
					auto range = ObserveDiscuss.equal_range(eve.fromDiscuss);
					for (auto it = range.first; it != range.second; it++)
					{
						if (it->second != eve.fromQQ)
						{
							mutexMsg.lock();
							SendMsgQueue.push(MsgType{ it->second,strAns,PrivateMsg });
							mutexMsg.unlock();
						}
					}
				}

			}
		}
		if (isHidden)
		{
			string strReply = strNickName + "进行了一次暗骰";
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromDiscuss,strReply,DiscussMsg });
			mutexMsg.unlock();
		}
	}
	else
	{
		if (isalpha(eve.message[intMsgCnt]))
		{
			mutexMsg.lock();
			SendMsgQueue.push(MsgType{ eve.fromDiscuss,"命令输入错误!",DiscussMsg });
			mutexMsg.unlock();
		}
	}
}

EVE_System_GroupMemberIncrease(__eventGroupMemberIncrease)
{
	if (beingOperateQQ != getLoginQQ() && WelcomeMsg.count(fromGroup))
	{
		string strReply = WelcomeMsg[fromGroup];
		while (strReply.find("{@}") != string::npos)
		{
			strReply.replace(strReply.find("{@}"), 3, CQ::code::at(beingOperateQQ));
		}
		while (strReply.find("{nick}") != string::npos)
		{
			strReply.replace(strReply.find("{nick}"), 6, getStrangerInfo(beingOperateQQ).nick);
		}
		while (strReply.find("{age}") != string::npos)
		{
			strReply.replace(strReply.find("{age}"), 5, to_string(getStrangerInfo(beingOperateQQ).age));
		}
		while (strReply.find("{sex}") != string::npos)
		{
			strReply.replace(strReply.find("{sex}"), 5, getStrangerInfo(beingOperateQQ).sex == 0 ? "男" : getStrangerInfo(beingOperateQQ).sex == 1 ? "女" : "未知");
		} 
		while (strReply.find("{qq}") != string::npos)
		{
			strReply.replace(strReply.find("{qq}"), 4, to_string(beingOperateQQ));
		}
		mutexMsg.lock();
		SendMsgQueue.push(MsgType{ fromGroup,strReply,GroupMsg });
		mutexMsg.unlock();
	}
	return 0;
}
EVE_Disable(__eventDisable)
{
	Enabled = false;
	ofstream ofstreamGroupName(strFileLoc + "GroupName.RDconf", ios::in | ios::trunc);
	for (auto it = GroupName.begin(); it != GroupName.end(); it++)
	{
		while (it->second.find(' ') != string::npos)it->second.replace(it->second.find(' '), 1, "{space}");
		while (it->second.find('\t') != string::npos)it->second.replace(it->second.find('\t'), 1, "{tab}");
		while (it->second.find('\n') != string::npos)it->second.replace(it->second.find('\n'), 1, "{endl}");
		while (it->second.find('\r') != string::npos)it->second.replace(it->second.find('\r'), 1, "{enter}");
		if (!(it->second).empty())
			ofstreamGroupName << it->first.first << " " << it->first.second << " " << it->second << std::endl;
	}
	ofstreamGroupName.close();
	ofstream ofstreamDiscussName(strFileLoc + "DiscussName.RDconf", ios::in | ios::trunc);
	for (auto it = DiscussName.begin(); it != DiscussName.end(); it++)
	{
		while (it->second.find(' ') != string::npos)it->second.replace(it->second.find(' '), 1, "{space}");
		while (it->second.find('\t') != string::npos)it->second.replace(it->second.find('\t'), 1, "{tab}");
		while (it->second.find('\n') != string::npos)it->second.replace(it->second.find('\n'), 1, "{endl}");
		while (it->second.find('\r') != string::npos)it->second.replace(it->second.find('\r'), 1, "{enter}");
		if (!(it->second).empty())
			ofstreamDiscussName << it->first.first << " " << it->first.second << " " << it->second << std::endl;
	}
	ofstreamDiscussName.close();

	ofstream ofstreamDisabledGroup(strFileLoc + "DisabledGroup.RDconf", ios::in | ios::trunc);
	for (auto it = DisabledGroup.begin(); it != DisabledGroup.end(); it++)
	{
		ofstreamDisabledGroup << *it << std::endl;
	}
	ofstreamDisabledGroup.close();

	ofstream ofstreamDisabledDiscuss(strFileLoc + "DisabledDiscuss.RDconf", ios::in | ios::trunc);
	for (auto it = DisabledDiscuss.begin(); it != DisabledDiscuss.end(); it++)
	{
		ofstreamDisabledDiscuss << *it << std::endl;
	}
	ofstreamDisabledDiscuss.close();
	ofstream ofstreamDisabledJRRPGroup(strFileLoc + "DisabledJRRPGroup.RDconf", ios::in | ios::trunc);
	for (auto it = DisabledJRRPGroup.begin(); it != DisabledJRRPGroup.end(); it++)
	{
		ofstreamDisabledJRRPGroup << *it << std::endl;
	}
	ofstreamDisabledJRRPGroup.close();

	ofstream ofstreamDisabledJRRPDiscuss(strFileLoc + "DisabledJRRPDiscuss.RDconf", ios::in | ios::trunc);
	for (auto it = DisabledJRRPDiscuss.begin(); it != DisabledJRRPDiscuss.end(); it++)
	{
		ofstreamDisabledJRRPDiscuss << *it << std::endl;
	}
	ofstreamDisabledJRRPDiscuss.close();

	ofstream ofstreamDisabledMEGroup(strFileLoc + "DisabledMEGroup.RDconf", ios::in | ios::trunc);
	for (auto it = DisabledMEGroup.begin(); it != DisabledMEGroup.end(); it++)
	{
		ofstreamDisabledMEGroup << *it << std::endl;
	}
	ofstreamDisabledMEGroup.close();

	ofstream ofstreamDisabledMEDiscuss(strFileLoc + "DisabledMEDiscuss.RDconf", ios::in | ios::trunc);
	for (auto it = DisabledMEDiscuss.begin(); it != DisabledMEDiscuss.end(); it++)
	{
		ofstreamDisabledMEDiscuss << *it << std::endl;
	}
	ofstreamDisabledMEDiscuss.close();

	ofstream ofstreamDisabledOBGroup(strFileLoc + "DisabledOBGroup.RDconf", ios::in | ios::trunc);
	for (auto it = DisabledOBGroup.begin(); it != DisabledOBGroup.end(); it++)
	{
		ofstreamDisabledOBGroup << *it << std::endl;
	}
	ofstreamDisabledOBGroup.close();

	ofstream ofstreamDisabledOBDiscuss(strFileLoc + "DisabledOBDiscuss.RDconf", ios::in | ios::trunc);
	for (auto it = DisabledOBDiscuss.begin(); it != DisabledOBDiscuss.end(); it++)
	{
		ofstreamDisabledOBDiscuss << *it << std::endl;
	}
	ofstreamDisabledOBDiscuss.close();

	ofstream ofstreamObserveGroup(strFileLoc + "ObserveGroup.RDconf", ios::in | ios::trunc);
	for (auto it = ObserveGroup.begin(); it != ObserveGroup.end(); it++)
	{
		ofstreamObserveGroup << it->first << " " << it->second << std::endl;
	}
	ofstreamObserveGroup.close();

	ofstream ofstreamObserveDiscuss(strFileLoc + "ObserveDiscuss.RDconf", ios::in | ios::trunc);
	for (auto it = ObserveDiscuss.begin(); it != ObserveDiscuss.end(); it++)
	{
		ofstreamObserveDiscuss << it->first << " " << it->second << std::endl;
	}
	ofstreamObserveDiscuss.close();
	ofstream ofstreamJRRP(strFileLoc + "JRRP.RDconf", ios::in | ios::trunc);
	for (auto it = JRRP.begin(); it != JRRP.end(); it++)
	{
		ofstreamJRRP << it->first << " " << it->second.Date << " " << it->second.RPVal << std::endl;
	}
	ofstreamJRRP.close();
	ofstream ofstreamCharacterProp(strFileLoc + "CharacterProp.RDconf", ios::in | ios::trunc);
	for (auto it = CharacterProp.begin(); it != CharacterProp.end(); it++) {
		for (auto it1 = it->second.cbegin(); it1 != it->second.cend(); it1++) {
			ofstreamCharacterProp << it->first.QQ << " " << it->first.Type << " " << it->first.GrouporDiscussID << " " << it1->first << " " << it1->second << std::endl;
		}
	}
	ofstreamCharacterProp.close();
	ofstream ofstreamDefault(strFileLoc + "Default.RDconf", ios::in | ios::trunc);
	for (auto it = DefaultDice.begin(); it != DefaultDice.end(); it++)
	{
		ofstreamDefault << it->first << " " << it->second << std::endl;
	}
	ofstreamDefault.close();

	ofstream ofstreamWelcomeMsg(strFileLoc + "WelcomeMsg.RDconf", ios::in | ios::trunc);
	for (auto it = WelcomeMsg.begin(); it != WelcomeMsg.end(); it++)
	{
		while (it->second.find(' ') != string::npos)it->second.replace(it->second.find(' '), 1, "{space}");
		while (it->second.find('\t') != string::npos)it->second.replace(it->second.find('\t'), 1, "{tab}");
		while (it->second.find('\n') != string::npos)it->second.replace(it->second.find('\n'), 1, "{endl}");
		while (it->second.find('\r') != string::npos)it->second.replace(it->second.find('\r'), 1, "{enter}");
		ofstreamWelcomeMsg << it->first << " " << it->second << std::endl;
	}
	ofstreamWelcomeMsg.close();
	JRRP.clear();
	DefaultDice.clear();
	GroupName.clear();
	DiscussName.clear();
	DisabledGroup.clear();
	DisabledDiscuss.clear();
	DisabledJRRPGroup.clear();
	DisabledJRRPDiscuss.clear();
	DisabledMEGroup.clear();
	DisabledMEDiscuss.clear();
	DisabledOBGroup.clear();
	DisabledOBDiscuss.clear();
	ObserveGroup.clear();
	ObserveDiscuss.clear();
	strFileLoc.clear();
	return 0;
}
EVE_Exit(__eventExit)
{
	if (!Enabled)
		return 0;
	ofstream ofstreamGroupName(strFileLoc + "GroupName.RDconf", ios::in | ios::trunc);
	for (auto it = GroupName.begin(); it != GroupName.end(); it++)
	{
		while (it->second.find(' ') != string::npos)it->second.replace(it->second.find(' '), 1, "{space}");
		while (it->second.find('\t') != string::npos)it->second.replace(it->second.find('\t'), 1, "{tab}");
		while (it->second.find('\n') != string::npos)it->second.replace(it->second.find('\n'), 1, "{endl}");
		while (it->second.find('\r') != string::npos)it->second.replace(it->second.find('\r'), 1, "{enter}");
		if (!(it->second).empty())
			ofstreamGroupName << it->first.first << " " << it->first.second << " " << it->second << std::endl;
	}
	ofstreamGroupName.close();
	ofstream ofstreamDiscussName(strFileLoc + "DiscussName.RDconf", ios::in | ios::trunc);
	for (auto it = DiscussName.begin(); it != DiscussName.end(); it++)
	{
		while (it->second.find(' ') != string::npos)it->second.replace(it->second.find(' '), 1, "{space}");
		while (it->second.find('\t') != string::npos)it->second.replace(it->second.find('\t'), 1, "{tab}");
		while (it->second.find('\n') != string::npos)it->second.replace(it->second.find('\n'), 1, "{endl}");
		while (it->second.find('\r') != string::npos)it->second.replace(it->second.find('\r'), 1, "{enter}");
		if (!(it->second).empty())
			ofstreamDiscussName << it->first.first << " " << it->first.second << " " << it->second << std::endl;
	}
	ofstreamDiscussName.close();

	ofstream ofstreamDisabledGroup(strFileLoc + "DisabledGroup.RDconf", ios::in | ios::trunc);
	for (auto it = DisabledGroup.begin(); it != DisabledGroup.end(); it++)
	{
		ofstreamDisabledGroup << *it << std::endl;
	}
	ofstreamDisabledGroup.close();

	ofstream ofstreamDisabledDiscuss(strFileLoc + "DisabledDiscuss.RDconf", ios::in | ios::trunc);
	for (auto it = DisabledDiscuss.begin(); it != DisabledDiscuss.end(); it++)
	{
		ofstreamDisabledDiscuss << *it << std::endl;
	}
	ofstreamDisabledDiscuss.close();
	ofstream ofstreamDisabledJRRPGroup(strFileLoc + "DisabledJRRPGroup.RDconf", ios::in | ios::trunc);
	for (auto it = DisabledJRRPGroup.begin(); it != DisabledJRRPGroup.end(); it++)
	{
		ofstreamDisabledJRRPGroup << *it << std::endl;
	}
	ofstreamDisabledJRRPGroup.close();

	ofstream ofstreamDisabledJRRPDiscuss(strFileLoc + "DisabledJRRPDiscuss.RDconf", ios::in | ios::trunc);
	for (auto it = DisabledJRRPDiscuss.begin(); it != DisabledJRRPDiscuss.end(); it++)
	{
		ofstreamDisabledJRRPDiscuss << *it << std::endl;
	}
	ofstreamDisabledJRRPDiscuss.close();

	ofstream ofstreamDisabledMEGroup(strFileLoc + "DisabledMEGroup.RDconf", ios::in | ios::trunc);
	for (auto it = DisabledMEGroup.begin(); it != DisabledMEGroup.end(); it++)
	{
		ofstreamDisabledMEGroup << *it << std::endl;
	}
	ofstreamDisabledMEGroup.close();

	ofstream ofstreamDisabledMEDiscuss(strFileLoc + "DisabledMEDiscuss.RDconf", ios::in | ios::trunc);
	for (auto it = DisabledMEDiscuss.begin(); it != DisabledMEDiscuss.end(); it++)
	{
		ofstreamDisabledMEDiscuss << *it << std::endl;
	}
	ofstreamDisabledMEDiscuss.close();

	ofstream ofstreamDisabledOBGroup(strFileLoc + "DisabledOBGroup.RDconf", ios::in | ios::trunc);
	for (auto it = DisabledOBGroup.begin(); it != DisabledOBGroup.end(); it++)
	{
		ofstreamDisabledOBGroup << *it << std::endl;
	}
	ofstreamDisabledOBGroup.close();

	ofstream ofstreamDisabledOBDiscuss(strFileLoc + "DisabledOBDiscuss.RDconf", ios::in | ios::trunc);
	for (auto it = DisabledOBDiscuss.begin(); it != DisabledOBDiscuss.end(); it++)
	{
		ofstreamDisabledOBDiscuss << *it << std::endl;
	}
	ofstreamDisabledOBDiscuss.close();

	ofstream ofstreamObserveGroup(strFileLoc + "ObserveGroup.RDconf", ios::in | ios::trunc);
	for (auto it = ObserveGroup.begin(); it != ObserveGroup.end(); it++)
	{
		ofstreamObserveGroup << it->first << " " << it->second << std::endl;
	}
	ofstreamObserveGroup.close();

	ofstream ofstreamObserveDiscuss(strFileLoc + "ObserveDiscuss.RDconf", ios::in | ios::trunc);
	for (auto it = ObserveDiscuss.begin(); it != ObserveDiscuss.end(); it++)
	{
		ofstreamObserveDiscuss << it->first << " " << it->second << std::endl;
	}
	ofstreamObserveDiscuss.close();
	ofstream ofstreamJRRP(strFileLoc + "JRRP.RDconf", ios::in | ios::trunc);
	for (auto it = JRRP.begin(); it != JRRP.end(); it++)
	{
		ofstreamJRRP << it->first << " " << it->second.Date << " " << it->second.RPVal << std::endl;
	}
	ofstreamJRRP.close();
	ofstream ofstreamCharacterProp(strFileLoc + "CharacterProp.RDconf", ios::in | ios::trunc);
	for (auto it = CharacterProp.begin(); it != CharacterProp.end(); it++) {
		for (auto it1 = it->second.cbegin(); it1 != it->second.cend(); it1++) {
			ofstreamCharacterProp << it->first.QQ << " " << it->first.Type << " " << it->first.GrouporDiscussID << " " << it1->first << " " << it1->second << std::endl;
		}
	}
	ofstreamCharacterProp.close();
	ofstream ofstreamDefault(strFileLoc + "Default.RDconf", ios::in | ios::trunc);
	for (auto it = DefaultDice.begin(); it != DefaultDice.end(); it++)
	{
		ofstreamDefault << it->first << " " << it->second << std::endl;
	}
	ofstreamDefault.close();

	ofstream ofstreamWelcomeMsg(strFileLoc + "WelcomeMsg.RDconf", ios::in | ios::trunc);
	for (auto it = WelcomeMsg.begin(); it != WelcomeMsg.end(); it++)
	{
		while (it->second.find(' ') != string::npos)it->second.replace(it->second.find(' '), 1, "{space}");
		while (it->second.find('\t') != string::npos)it->second.replace(it->second.find('\t'), 1, "{tab}");
		while (it->second.find('\n') != string::npos)it->second.replace(it->second.find('\n'), 1, "{endl}");
		while (it->second.find('\r') != string::npos)it->second.replace(it->second.find('\r'), 1, "{enter}");
		ofstreamWelcomeMsg << it->first << " " << it->second << std::endl;
	}
	ofstreamWelcomeMsg.close();
	return 0;
}
MUST_AppInfo_RETURN(CQAPPID);
