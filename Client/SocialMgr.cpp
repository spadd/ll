/********************************************************************
	created:	2008/12/10
	created:	10:12:2008   11:28
	filename: 	d:\loong_client\Client\SocialMgr.cpp
	file path:	d:\loong_client\Client
	file base:	SocialMgr
	file ext:	cpp
	author:		leili
	
	purpose:	好友管理器实现
*********************************************************************/

#include "stdafx.h"
#include "SocialMgr.h"
#include "SocialListFrame.h"
#include "PlayerNameTab.h"
#include "SocialEvent.h"
#include "CombatSysUtil.h"
#include "ItemProtoData.h"
#include "RoleMgr.h"
#include "LocalPlayer.h"
#include "ChatEvent.h"
#include "SocialReply.h"
#include "ChatFrame.h"
#include "AudioSys.h"
#include "..\WorldDefine\SocialDef.h"
#include "MallGiftFrame.h"
#include "..\WorldDefine\msg_bloodbrother.h"
#include "EffectMgr.h"
namespace
{
	const float G_GiftChoiceTime = 30.0f;	// 选择是否接收礼物时间30秒
	const float G_InviteChoiceTime = 60.0f;	// 选择是否接收邀请时间30秒

	void CreateDir(const TCHAR* Path)
	{
		TCHAR DirName[256];
		const TCHAR* p = Path;
		TCHAR* q = DirName;	

		while(*p)
		{
			if ((_T('\\') == *p) || (_T('/') == *p))
			{
				if (_T(':') != *(p-1))
				{
					CreateDirectory(DirName, NULL);
				}
			}
			*q++ = *p++;
			*q = _T('\0');
		}
		CreateDirectory(DirName, NULL);
	}
}

SocialMgr::SocialMgr( void )
: m_Trunk(this)
, m_pData(0)
, m_pReply(0)
, m_pListFrame(0)
, m_giftGlobalID(GT_INVALID)
, m_giftTypeID(GT_INVALID)
, m_giftTiming(false)
, m_timeSelStart(0.0f)
, m_sendRoleID(GT_INVALID)
, m_autoReply(false)
, m_showOutline(false)
, m_blackID(GT_INVALID)
, m_mountRoleID(GT_INVALID)
, m_inviteTiming(false)
, m_timeInvStart(0.0f)
, m_isUsingGift(false)
, tagBloodBrotherTime(0)
, m_dwTeacherID(GT_INVALID)
{

}

SocialMgr::~SocialMgr( void )
{

}

BOOL SocialMgr::Init( GameFrameMgr* pMgr, GameFrame* pFather, DWORD dwParam )
{
	m_pData = new SocialData();
	if( !P_VALID(m_pData) )
		return FALSE;

	// 请求发送好友列表及黑名单
	/*tagNC_GetRoleInitState msg;
	msg.eType = ERIT_FrindAndEnemy;
	m_pZoneSession->Send(&msg);*/

	// 注册网络消息
	m_pCmdMgr->Register("NS_SendFriendBlackList",	(NETMSGPROC)m_Trunk.sfp2(&SocialMgr::NetRecvMessage_SendFriendBlackList),	_T("NS_SendFriendBlackList"));
	m_pCmdMgr->Register("NS_UpdateFriGroup",		(NETMSGPROC)m_Trunk.sfp2(&SocialMgr::NetRecvMessage_MoveToFriend),			_T("NS_UpdateFriGroup"));
	m_pCmdMgr->Register("NS_MoveToBlackList",		(NETMSGPROC)m_Trunk.sfp2(&SocialMgr::NetRecvMessage_MoveToBlackList),		_T("NS_MoveToBlackList"));
	m_pCmdMgr->Register("NS_RoleMakeFriend",		(NETMSGPROC)m_Trunk.sfp2(&SocialMgr::NetRecvMessage_AddFriend),				_T("NS_RoleMakeFriend"));
	m_pCmdMgr->Register("NS_RoleCancelFriend",		(NETMSGPROC)m_Trunk.sfp2(&SocialMgr::NetRecvMessage_DeleteFriend),			_T("NS_RoleCancelFriend"));
	m_pCmdMgr->Register("NS_DeleteBlackList",		(NETMSGPROC)m_Trunk.sfp2(&SocialMgr::NetRecvMessage_DeleteBlack),			_T("NS_DeleteBlackList"));
	m_pCmdMgr->Register("NS_SendGiftToFri",			(NETMSGPROC)m_Trunk.sfp2(&SocialMgr::NetRecvMessage_GetGift),				_T("NS_SendGiftToFri"));
	m_pCmdMgr->Register("NS_SendGiftBroadcast",		(NETMSGPROC)m_Trunk.sfp2(&SocialMgr::NetRecvMessage_SendGiftBroadcast),		_T("NS_SendGiftBroadcast"));
	m_pCmdMgr->Register("NS_LoginToFriend",			(NETMSGPROC)m_Trunk.sfp2(&SocialMgr::NetRecvMessage_FriendLogIn),			_T("NS_LoginToFriend"));
	m_pCmdMgr->Register("NS_LogoutToFriend",		(NETMSGPROC)m_Trunk.sfp2(&SocialMgr::NetRecvMessage_FriendLogOut),			_T("NS_LogoutToFriend"));
	m_pCmdMgr->Register("NS_UpdateFriEnemyState",	(NETMSGPROC)m_Trunk.sfp2(&SocialMgr::NetRecvMessage_UpdateFriendState),		_T("NS_UpdateFriEnemyState"));
	m_pCmdMgr->Register("NS_SendGiftToSender",		(NETMSGPROC)m_Trunk.sfp2(&SocialMgr::NetRecvMessage_SendGiftError),			_T("NS_SendGiftToSender"));
	m_pCmdMgr->Register("NS_UpdateFriValue",		(NETMSGPROC)m_Trunk.sfp2(&SocialMgr::NetRecvMessage_UpdateFriendValue),		_T("NS_UpdateFriValue"));
	m_pCmdMgr->Register("NS_MakeFriNotice",			(NETMSGPROC)m_Trunk.sfp2(&SocialMgr::NetRecvMessage_FriendNotice),			_T("NS_MakeFriNotice"));
	m_pCmdMgr->Register("NS_MountOnInvite",			(NETMSGPROC)m_Trunk.sfp2(&SocialMgr::NetRecvMessage_MountOnInvite),			_T("NS_MountOnInvite"));
	m_pCmdMgr->Register("NS_InsertEnemy",			(NETMSGPROC)m_Trunk.sfp2(&SocialMgr::NetRecvMessage_InsertEnemy),			_T("NS_InsertEnemy"));
	m_pCmdMgr->Register("NS_DeleteEnemy",			(NETMSGPROC)m_Trunk.sfp2(&SocialMgr::NetRecvMessage_DeleteEnemy),			_T("NS_DeleteEnemy"));
	m_pCmdMgr->Register("NS_EnemyLogin",			(NETMSGPROC)m_Trunk.sfp2(&SocialMgr::NetRecvMessage_NS_EnemyLogin),			_T("NS_EnemyLogin"));
	m_pCmdMgr->Register("NS_TryMakeBrother",			(NETMSGPROC)m_Trunk.sfp2(&SocialMgr::NetRecvMessage_NS_TryMakeBrother),			_T("NS_TryMakeBrother"));
	m_pCmdMgr->Register("NS_BrotherLeftSecond",			(NETMSGPROC)m_Trunk.sfp2(&SocialMgr::NetRecvMessage_NS_BrotherLeftSecond),			_T("NS_BrotherLeftSecond"));
	m_pCmdMgr->Register("NS_BrotherEnd",			(NETMSGPROC)m_Trunk.sfp2(&SocialMgr::NetRecvMessage_NS_BrotherEnd),			_T("NS_BrotherEnd"));
	// 注册游戏事件
	pMgr->RegisterEventHandle(_T("Open_Sociality"),			(FRAMEEVENTPROC)m_Trunk.sfp1(&SocialMgr::OnEvent_OpenList));
	pMgr->RegisterEventHandle(_T("Close_Sociality"),		(FRAMEEVENTPROC)m_Trunk.sfp1(&SocialMgr::OnEvent_CloseList));
	pMgr->RegisterEventHandle(_T("SocialEvent_AddFriend"),	(FRAMEEVENTPROC)m_Trunk.sfp1(&SocialMgr::OnEvent_AddFriend));
	pMgr->RegisterEventHandle(_T("SocialEvent_AddBlack"),	(FRAMEEVENTPROC)m_Trunk.sfp1(&SocialMgr::OnEvent_AddBlack));
	pMgr->RegisterEventHandle(_T("SocialEvent_UseGift"),	(FRAMEEVENTPROC)m_Trunk.sfp1(&SocialMgr::OnEvent_UseGift));
	pMgr->RegisterEventHandle(_T("SocialEvent_GiveGiftTo"),	(FRAMEEVENTPROC)m_Trunk.sfp1(&SocialMgr::OnEvent_GiveGiftTo));
	pMgr->RegisterEventHandle(_T("SocialEvent_GetGift"),	(FRAMEEVENTPROC)m_Trunk.sfp1(&SocialMgr::OnEvent_GetGift));
	pMgr->RegisterEventHandle(_T("tagRoleGetNameEvent"),	(FRAMEEVENTPROC)m_Trunk.sfp1(&SocialMgr::OnEvent_GetRoleName));
	pMgr->RegisterEventHandle(_T("SocialEvent_MsgAddBlack"),(FRAMEEVENTPROC)m_Trunk.sfp1(&SocialMgr::OnEvent_MsgAddBlack));
	pMgr->RegisterEventHandle(_T("MountEvent_Invite"),		(FRAMEEVENTPROC)m_Trunk.sfp1(&SocialMgr::OnEvent_Mount));
	pMgr->RegisterEventHandle(_T("tagInitLPAttEvent"),		(FRAMEEVENTPROC)m_Trunk.sfp1(&SocialMgr::OnEvent_LPAttIsOK));
	pMgr->RegisterEventHandle(_T("Event_RoleNotExist"),		(FRAMEEVENTPROC)m_Trunk.sfp1(&SocialMgr::OnEvent_GetRoleIDError));
	pMgr->RegisterEventHandle(_T("Socialevent_IsAddFriendMsgBox"),	(FRAMEEVENTPROC)m_Trunk.sfp1(&SocialMgr::OnEvent_IsAddFriendMsgBox));    //注册显示是否添加对方为好友提示框的事件
	pMgr->RegisterEventHandle(_T("BloodBrother"),		(FRAMEEVENTPROC)m_Trunk.sfp1(&SocialMgr::OnEvent_BloodBrother));

	return GameFrame::Init(pMgr,pFather,dwParam);
}

BOOL SocialMgr::Destroy()
{
	// 清理现场
	m_logInDelayNames.clear();
	m_logOutDelayNames.clear();
	m_noticeDelayNames.clear();
	m_giftDelayBCs.clear();
	m_giveGiftDelayIDs.clear();
	m_getGiftDelayNames.clear();
	m_getMountDelayNames.clear();
	m_giftList.clear();
	m_mountList.clear();

	SAFE_DELETE(m_pReply);
	SAFE_DELETE(m_pData);

	LPVOID pObj = GetObj("GiftChoiceBox");
	if( P_VALID(pObj) )
		KillObj("GiftChoiceBox");

	if( P_VALID(m_pListFrame) )
	{
		//m_pMgr->AddToDestroyList(m_pListFrame);
		m_pListFrame = NULL;
	}

	// 注销网络消息
	m_pCmdMgr->UnRegister("NS_SendFriendBlackList",	(NETMSGPROC)m_Trunk.sfp2(&SocialMgr::NetRecvMessage_SendFriendBlackList));
	m_pCmdMgr->UnRegister("NS_UpdateFriGroup",		(NETMSGPROC)m_Trunk.sfp2(&SocialMgr::NetRecvMessage_MoveToFriend));
	m_pCmdMgr->UnRegister("NS_MoveToBlackList",		(NETMSGPROC)m_Trunk.sfp2(&SocialMgr::NetRecvMessage_MoveToBlackList));
	m_pCmdMgr->UnRegister("NS_RoleMakeFriend",		(NETMSGPROC)m_Trunk.sfp2(&SocialMgr::NetRecvMessage_AddFriend));
	m_pCmdMgr->UnRegister("NS_RoleCancelFriend",	(NETMSGPROC)m_Trunk.sfp2(&SocialMgr::NetRecvMessage_DeleteFriend));
	m_pCmdMgr->UnRegister("NS_DeleteBlackList",		(NETMSGPROC)m_Trunk.sfp2(&SocialMgr::NetRecvMessage_DeleteBlack));
	m_pCmdMgr->UnRegister("NS_SendGiftToFri",		(NETMSGPROC)m_Trunk.sfp2(&SocialMgr::NetRecvMessage_GetGift));
	m_pCmdMgr->UnRegister("NS_SendGiftBroadcast",	(NETMSGPROC)m_Trunk.sfp2(&SocialMgr::NetRecvMessage_SendGiftBroadcast));
	m_pCmdMgr->UnRegister("NS_LoginToFriend",		(NETMSGPROC)m_Trunk.sfp2(&SocialMgr::NetRecvMessage_FriendLogIn));
	m_pCmdMgr->UnRegister("NS_LogoutToFriend",		(NETMSGPROC)m_Trunk.sfp2(&SocialMgr::NetRecvMessage_FriendLogOut));
	m_pCmdMgr->UnRegister("NS_UpdateFriEnemyState",		(NETMSGPROC)m_Trunk.sfp2(&SocialMgr::NetRecvMessage_UpdateFriendState));
	m_pCmdMgr->UnRegister("NS_SendGiftToSender",	(NETMSGPROC)m_Trunk.sfp2(&SocialMgr::NetRecvMessage_SendGiftError));
	m_pCmdMgr->UnRegister("NS_UpdateFriValue",		(NETMSGPROC)m_Trunk.sfp2(&SocialMgr::NetRecvMessage_UpdateFriendValue));
	m_pCmdMgr->UnRegister("NS_MakeFriNotice",		(NETMSGPROC)m_Trunk.sfp2(&SocialMgr::NetRecvMessage_FriendNotice));
	m_pCmdMgr->UnRegister("NS_MountOnInvite",		(NETMSGPROC)m_Trunk.sfp2(&SocialMgr::NetRecvMessage_MountOnInvite));
	m_pCmdMgr->UnRegister("NS_InsertEnemy",		(NETMSGPROC)m_Trunk.sfp2(&SocialMgr::NetRecvMessage_InsertEnemy));
	m_pCmdMgr->UnRegister("NS_DeleteEnemy",		(NETMSGPROC)m_Trunk.sfp2(&SocialMgr::NetRecvMessage_DeleteEnemy));
	m_pCmdMgr->UnRegister("NS_EnemyLogin",		(NETMSGPROC)m_Trunk.sfp2(&SocialMgr::NetRecvMessage_NS_EnemyLogin));
	m_pCmdMgr->UnRegister("NS_TryMakeBrother",		(NETMSGPROC)m_Trunk.sfp2(&SocialMgr::NetRecvMessage_NS_TryMakeBrother));
	m_pCmdMgr->UnRegister("NS_BrotherLeftSecond",		(NETMSGPROC)m_Trunk.sfp2(&SocialMgr::NetRecvMessage_NS_BrotherLeftSecond));
	m_pCmdMgr->UnRegister("NS_BrotherEnd",		(NETMSGPROC)m_Trunk.sfp2(&SocialMgr::NetRecvMessage_NS_BrotherEnd));

	// 注销游戏事件
	m_pMgr->UnRegisterEventHandler( _T("Open_Sociality"),			(FRAMEEVENTPROC)m_Trunk.sfp1(&SocialMgr::OnEvent_OpenList));
	m_pMgr->UnRegisterEventHandler( _T("Close_Sociality"),			(FRAMEEVENTPROC)m_Trunk.sfp1(&SocialMgr::OnEvent_CloseList));
	m_pMgr->UnRegisterEventHandler( _T("SocialEvent_AddFriend"),	(FRAMEEVENTPROC)m_Trunk.sfp1(&SocialMgr::OnEvent_AddFriend));
	m_pMgr->UnRegisterEventHandler( _T("SocialEvent_AddBlack"),		(FRAMEEVENTPROC)m_Trunk.sfp1(&SocialMgr::OnEvent_AddBlack));
	m_pMgr->UnRegisterEventHandler( _T("SocialEvent_UseGift"),		(FRAMEEVENTPROC)m_Trunk.sfp1(&SocialMgr::OnEvent_UseGift));
	m_pMgr->UnRegisterEventHandler( _T("SocialEvent_GiveGiftTo"),	(FRAMEEVENTPROC)m_Trunk.sfp1(&SocialMgr::OnEvent_GiveGiftTo));
	m_pMgr->UnRegisterEventHandler( _T("SocialEvent_GetGift"),		(FRAMEEVENTPROC)m_Trunk.sfp1(&SocialMgr::OnEvent_GetGift));
	m_pMgr->UnRegisterEventHandler( _T("tagRoleGetNameEvent"),		(FRAMEEVENTPROC)m_Trunk.sfp1(&SocialMgr::OnEvent_GetRoleName));
	m_pMgr->UnRegisterEventHandler(_T("SocialEvent_MsgAddBlack"),	(FRAMEEVENTPROC)m_Trunk.sfp1(&SocialMgr::OnEvent_MsgAddBlack));
	m_pMgr->UnRegisterEventHandler(_T("MountEvent_Invite"),			(FRAMEEVENTPROC)m_Trunk.sfp1(&SocialMgr::OnEvent_Mount));
	m_pMgr->UnRegisterEventHandler( _T("tagInitLPAttEvent"),		(FRAMEEVENTPROC)m_Trunk.sfp1(&SocialMgr::OnEvent_LPAttIsOK));
	m_pMgr->UnRegisterEventHandler(_T("Event_RoleNotExist"),		(FRAMEEVENTPROC)m_Trunk.sfp1(&SocialMgr::OnEvent_GetRoleIDError));
    m_pMgr->UnRegisterEventHandler(_T("Socialevent_IsAddFriendMsgBox"),		(FRAMEEVENTPROC)m_Trunk.sfp1(&SocialMgr::OnEvent_IsAddFriendMsgBox));     //注销添加好友提示框事件
	m_pMgr->UnRegisterEventHandler(_T("BloodBrother"),		(FRAMEEVENTPROC)m_Trunk.sfp1(&SocialMgr::OnEvent_BloodBrother));

	if(P_VALID(GetObj("BloodBrotherEnd")))
		KillObj("BloodBrotherEnd");

	return GameFrame::Destroy();
}

VOID SocialMgr::Update()
{
	GameFrame::Update();

	if( m_giftTiming )
	{
		if( Kernel::Inst()->GetAccumTime() - m_timeSelStart > G_GiftChoiceTime )//超过30秒仍没有选择
		{
			m_giftTiming = false;
			SendGiftReply( m_giftTypeID, FALSE );

			KillObj("GiftChoiceBox");
		}
	}
	else//查看礼物列表中是否有赠送的礼物
	{
		if( m_giftList.size() > 0 )
		{
			tagNS_SendGiftToFri& msg = *m_giftList.begin();

			LPCTSTR szName = PlayerNameTab::Inst()->FindNameByID(msg.dwSrcRoleID);
			if( _tcslen(szName) > 0 )
				ShowGetGift( &msg );
			else
				m_getGiftDelayNames.push_back( msg );

			m_giftList.erase( m_giftList.begin() );
		}
	}

	if( m_inviteTiming )
	{
		Role *pTarget = NULL;
		map<DWORD,Role*>::const_iterator finder = RoleMgr::Inst()->GetRoleMap().find(m_mountRoleID);
		if( finder != RoleMgr::Inst()->GetRoleMap().end() )
			pTarget = finder->second;

		if( (Kernel::Inst()->GetAccumTime() - m_timeInvStart > G_InviteChoiceTime)//超过60秒仍没有选择
			|| !P_VALID(pTarget)
			|| (Vec3DistSq(RoleMgr::Inst()->GetLocalPlayer()->GetPos(), pTarget->GetPos()) > 250000) )//距离超过10格
		{
			m_inviteTiming =false;
			SendMountReply( FALSE );

			KillObj("MountInviteBox");
		}
	}
	else
	{
		if( m_mountList.size() > 0 )
		{
			tagNS_MountOnInvite& msg = *m_mountList.begin();

			LPCTSTR szName = PlayerNameTab::Inst()->FindNameByID(msg.dwSrcRoleID);
			if( _tcslen(szName) > 0 )
				ShowMountInvite( &msg );
			else
				m_getMountDelayNames.push_back( msg );

			m_mountList.erase( m_mountList.begin() );
		}
	}
}

//-----------------------------------------------------------------------------
// 注册子Frame
//-----------------------------------------------------------------------------
void SocialMgr::RegisterFrames( GameFrameMgr* pMgr )
{
	pMgr->Register(TWrap<SocialListFrame>(), _T("SocialListFrame"));
}

//-----------------------------------------------------------------------------
// 处理网络消息NS_SendFriendBlackList
//-----------------------------------------------------------------------------
DWORD SocialMgr::NetRecvMessage_SendFriendBlackList( tagNS_SendFriendBlackList* pMsg, DWORD pParam )
{
	for( INT i=0; i<pMsg->nNum; i++ )
	{
		// 获得玩家名称
		PlayerNameTab::Inst()->FindNameByID( pMsg->FriendInfo[i].dwFriendID );

		switch( pMsg->FriendInfo[i].byGroup )
		{
		case 1:
			m_pData->m_friendsArray1.push_back( pMsg->FriendInfo[i] );
			break;
		case 2:
			m_pData->m_friendsArray2.push_back( pMsg->FriendInfo[i] );
			break;
		case 3:
			m_pData->m_friendsArray3.push_back( pMsg->FriendInfo[i] );
			break;
		case 4:
			m_pData->m_friendsArray4.push_back( pMsg->FriendInfo[i] );
			break;
		}
	}

	for( INT i=0; i<MAX_BLACKLIST; i++ )
	{
		if( !P_VALID(pMsg->dwRolesID[i]) )
			break;

		// 获得玩家名称
		PlayerNameTab::Inst()->FindNameByID( pMsg->dwRolesID[i] );

		m_pData->m_blackArray.push_back( pMsg->dwRolesID[i] );
	}

	m_pData->m_foeArray.clear();
	for( INT i=0; i<MAX_ENEMYNUM; i++ )
	{
		tagEnemyInfo& enemyInfo = pMsg->dwEnemysID[i];
		if( !P_VALID(enemyInfo.dwEnemyID) )
			break;

		// 获得玩家名称
		PlayerNameTab::Inst()->FindNameByID( enemyInfo.dwEnemyID );

		m_pData->m_foeArray.push_back( enemyInfo );
	}
	// 刷新好友列表显示
	if( P_VALID(m_pListFrame) )
		m_pListFrame->RefreshList( SocialData::ELT_FoeList );

	tagNC_GetRoleInitState msg;
	msg.eType = ERIT_Money;
	TObjRef<NetSession>()->Send(&msg);

	return 0;
}

//-----------------------------------------------------------------------------
// 处理网络消息NS_UpdateFriGroup
//-----------------------------------------------------------------------------
DWORD SocialMgr::NetRecvMessage_MoveToFriend( tagNS_UpdateFriGroup* pMsg, DWORD pParam )
{
	switch( pMsg->dwErrorCode )
	{
	case E_Success:
		{
			const tagFriendInfo* pFriendInfo = m_pData->GetFriendPtrByID( (SocialData::EListType)pMsg->byOldGroup, pMsg->dwDestRoleID );
			if( P_VALID(pFriendInfo) )
			{
				m_pData->AddToFriendList( (SocialData::EListType)pMsg->byNewGroup, *pFriendInfo );

				m_pData->DeleteFromFriendList( (SocialData::EListType)pMsg->byOldGroup, pMsg->dwDestRoleID );
			}

			// 刷新好友列表显示
			if( P_VALID(m_pListFrame) )
				m_pListFrame->RefreshList( (SocialData::EListType)pMsg->byOldGroup );
		
			tagRefreshFriendsList event(_T("tagRefreshFriendsList"), NULL);
			m_pFrameMgr->SendEvent(&event);
		}
		break;

	default:
		break;
	}

	return 0;
}

//-----------------------------------------------------------------------------
// 处理网络消息NS_MoveToBlackList
//-----------------------------------------------------------------------------
DWORD SocialMgr::NetRecvMessage_MoveToBlackList( tagNS_MoveToBlackList* pMsg, DWORD pParam )
{
	switch( pMsg->dwErrorCode )
	{
	case E_Success:
		{
			m_pData->AddToBlackList( pMsg->dwDestRoleID );
			m_pData->DeleteFromFriendList( (SocialData::EListType)pMsg->byOldGroup, pMsg->dwDestRoleID );

			// 刷新好友列表显示
			if( P_VALID(m_pListFrame) )
				m_pListFrame->RefreshList();
		}
		break;
    case E_Friend_CanNotDel_Spouse:
        {
            CombatSysUtil::Inst()->ShowScreenCenterMsg(g_StrTable[_T("E_Friend_CanNotDel_Spouse")]);
        }

	default:
		break;
	}

	return 0;
}

//-----------------------------------------------------------------------------
// 处理网络消息NS_RoleMakeFriend
//-----------------------------------------------------------------------------
DWORD SocialMgr::NetRecvMessage_AddFriend( tagNS_RoleMakeFriend* pMsg, DWORD pParam )
{
	switch( pMsg->dwErrorCode )
	{
	case E_Success:
		{
			tagFriendInfo info;
			info.dwFriendID = pMsg->dwDestRoleID;
			info.bOnline	= pMsg->bOnline==TRUE;
			info.byGroup	= 1;//默认放到第一组
			info.nLevel		= pMsg->nLevel;
			info.dwFriVal	= 0;
			info.nVIPLevel  = pMsg->nVipLevel;

			m_pData->AddToFriendList( SocialData::ELT_Friend1, info );

			// 刷新好友列表显示
			if( P_VALID(m_pListFrame) )
				m_pListFrame->RefreshList( SocialData::ELT_Friend1 );

			tagRefreshFriendsList event(_T("tagRefreshFriendsList"), NULL);
			m_pFrameMgr->SendEvent(&event);
		}
		break;

	case E_Friend_Target_Not_Exist:
		{
			tagSocialInfoEvent chatInfo(_T("tagSocialInfoEvent"), NULL);
			chatInfo.strInfo = g_StrTable[_T("Social_RoleIsNotExist")];
			m_pMgr->SendEvent(&chatInfo);
		}
		break;
	case E_Friend_SystemRefuse:
		{
			CombatSysUtil::Inst()->ShowScreenCenterMsg(g_StrTable[_T("Social_RoleBeForbiden")]);
		}
		break;

	default:
		break;
	}

	return 0;
}

//-----------------------------------------------------------------------------
// 处理网络消息NS_RoleCancelFriend
//-----------------------------------------------------------------------------
DWORD SocialMgr::NetRecvMessage_DeleteFriend( tagNS_RoleCancelFriend* pMsg, DWORD pParam )
{
	switch( pMsg->dwErrorCode )
	{
	case E_Success:
		{
			m_pData->DeleteFromFriendList( (SocialData::EListType)pMsg->byGroup, pMsg->dwDestRoleID );

			// 刷新好友列表显示
			if( P_VALID(m_pListFrame) )
				m_pListFrame->RefreshList( (SocialData::EListType)pMsg->byGroup );
			
			tagRefreshFriendsList event(_T("tagRefreshFriendsList"), NULL);
			m_pFrameMgr->SendEvent(&event);
		}
		break;
    case E_Friend_CanNotDel_Spouse:
        {
            CombatSysUtil::Inst()->ShowScreenCenterMsg(g_StrTable[_T("Social_CanNotDelSpouse")]);
        }
        break;

	default:
		break;
	}

	return 0;
}

//-----------------------------------------------------------------------------
// 处理网络消息NS_DeleteBlackList
//-----------------------------------------------------------------------------
DWORD SocialMgr::NetRecvMessage_DeleteBlack( tagNS_DeleteBlackList* pMsg, DWORD pParam )
{
	switch( pMsg->dwErrorCode )
	{
	case E_Success:
		{
			m_pData->DeleteFromBlackList( pMsg->dwDestRoleID );

			// 刷新好友列表显示
			if( P_VALID(m_pListFrame) )
				m_pListFrame->RefreshList( SocialData::ELT_BlackList );
		}
		break;

	default:
		break;
	}

	return 0;
}

//-----------------------------------------------------------------------------
// 处理网络消息NS_LoginToFriend
//-----------------------------------------------------------------------------
DWORD SocialMgr::NetRecvMessage_FriendLogIn( tagNS_LoginToFriend* pMsg, DWORD pParam )
{
	LPCTSTR szName = PlayerNameTab::Inst()->FindNameByID( pMsg->dwRoleID );
	if( _tcslen(szName) > 0 )
		ShowFriendLogInInfo( pMsg->dwRoleID, szName );
	else
		m_logInDelayNames.push_back( pMsg->dwRoleID );

	//好友上线提示
	TObjRef<AudioSys>()->Play2DSound( _T("friend_login") );
	return 0;
}

//-----------------------------------------------------------------------------
// 处理网络消息NS_LogoutToFriend
//-----------------------------------------------------------------------------
DWORD SocialMgr::NetRecvMessage_FriendLogOut( tagNS_LogoutToFriend* pMsg, DWORD pParam )
{
	LPCTSTR szName = PlayerNameTab::Inst()->FindNameByID( pMsg->dwRoleID );
	if( P_VALID(m_pListFrame) )
	{
		m_pListFrame->ShowFunList(FALSE);
	}
	if( _tcslen(szName) > 0 )
		ShowFriendLogOutInfo( pMsg->dwRoleID, szName );
	else
		m_logOutDelayNames.push_back( pMsg->dwRoleID );

	return 0;
}

//-----------------------------------------------------------------------------
// 处理网络消息NS_SendGift
//-----------------------------------------------------------------------------
DWORD SocialMgr::NetRecvMessage_GetGift( tagNS_SendGiftToFri* pMsg, DWORD pParam )
{
	m_giftList.push_back( *pMsg );

	return 0;
}

//-----------------------------------------------------------------------------
// 处理网络消息NS_SendGiftBroadcast
//-----------------------------------------------------------------------------
DWORD SocialMgr::NetRecvMessage_SendGiftBroadcast( tagNS_SendGiftBroadcast* pMsg, DWORD pParam )
{
	LPCTSTR srcName = PlayerNameTab::Inst()->FindNameByID( pMsg->dwSrcRoleID );
	LPCTSTR destName = PlayerNameTab::Inst()->FindNameByID( pMsg->dwDestRoleID );

	if( _tcslen(srcName) > 0 && _tcslen(destName) > 0 )
		ShowGiftBroadcast( pMsg );
	else
	{
		tagBC bc;
		if( _tcslen(srcName) > 0 )
			bc.srcRoleID = GT_INVALID;
		else
			bc.srcRoleID = pMsg->dwSrcRoleID;

		if( _tcslen(destName) > 0 )
			bc.destRoleID = GT_INVALID;
		else
			bc.destRoleID = pMsg->dwDestRoleID;

		bc.msg = *pMsg;
		m_giftDelayBCs.push_back(bc);
	}

	return 0;
}

//-----------------------------------------------------------------------------
// 处理网络消息NS_UpdateFriEnemyState
//-----------------------------------------------------------------------------
DWORD SocialMgr::NetRecvMessage_UpdateFriendState( tagNS_UpdateFriEnemyState* pMsg, DWORD pParam )
{
	for( INT i=0; i<pMsg->nNum; i++ )
	{
		tagFriendInfo *pInfo = m_pData->GetFriend( pMsg->FriUpdate[i].dwRoleID );
		if( P_VALID(pInfo) )
		{
			pInfo->bOnline	= pMsg->FriUpdate[i].bOnline;
			pInfo->nLevel	= pMsg->FriUpdate[i].nLevel;
			pInfo->nVIPLevel = pMsg->FriUpdate[i].nVIPLevel;
		}
	}

	for( INT i=0; i<MAX_ENEMYNUM; i++ )
	{
		bool isInFoeArray = FALSE; //默认名单里面有此人
		tagEnemyInfo& enemyInfo = pMsg->dwEnemysID[i];
		for ( INT j = 0; j < (INT)m_pData->m_foeArray.size();j++)
		{
			tagEnemyInfo *foeInfo = &(m_pData->m_foeArray[j]);
			//如果ID已存在就直接更新等级，不重新写入
			if ( foeInfo->dwEnemyID == enemyInfo.dwEnemyID )
			{
				isInFoeArray = TRUE;
				foeInfo->nLevel = enemyInfo.nLevel;
				break;
			}
		}

		if( !P_VALID(enemyInfo.dwEnemyID) || isInFoeArray == TRUE )
			break;
		// 获得玩家名称
		PlayerNameTab::Inst()->FindNameByID( enemyInfo.dwEnemyID );
		m_pData->m_foeArray.push_back( enemyInfo );
		
	}

	if( P_VALID(m_pListFrame) )
	{
		m_pListFrame->RefreshList();
	}

	return 0;
}

//-----------------------------------------------------------------------------
// 处理网络消息NS_UpdateFriValue
//-----------------------------------------------------------------------------
DWORD SocialMgr::NetRecvMessage_UpdateFriendValue( tagNS_UpdateFriValue* pMsg, DWORD pParam )
{
	tagFriendInfo *pInfo = m_pData->GetFriend( pMsg->dwRoleID );
	if( P_VALID(pInfo) )
	{
		pInfo->dwFriVal = pMsg->nFriVal;
	}

	if( P_VALID(m_pListFrame) )
	{
		m_pListFrame->RefreshList();
	}

	return 0;
}

//-----------------------------------------------------------------------------
// 处理网络消息NS_MakeFriNotice
//-----------------------------------------------------------------------------
DWORD SocialMgr::NetRecvMessage_FriendNotice( tagNS_MakeFriNotice* pMsg, DWORD pParam )
{
	LPCTSTR szName = PlayerNameTab::Inst()->FindNameByID( pMsg->dwRoleID );
	if( _tcslen( szName ) > 0 )
		ShowFriendNotice( szName );
	else
		m_noticeDelayNames.push_back( pMsg->dwRoleID );

	return 0;
}

//-----------------------------------------------------------------------------
// 处理网络消息NS_SendGiftToSender
//-----------------------------------------------------------------------------
DWORD SocialMgr::NetRecvMessage_SendGiftError( tagNS_SendGiftToSender* pMsg, DWORD pParam )
{
	switch( pMsg->dwErrorCode )
	{
	case E_Gift_Not_Exit:
		{
			tagSocialInfoEvent chatInfo(_T("tagSocialInfoEvent"), NULL);
			chatInfo.strInfo = g_StrTable[_T("Social_ServerError_GiftNotExist")];
			m_pMgr->SendEvent(&chatInfo);
		}
		break;

	case E_Gift_Not_Both_Friend:
		{
			tagSocialInfoEvent chatInfo(_T("tagSocialInfoEvent"), NULL);
			chatInfo.strInfo = g_StrTable[_T("Social_ServerError_NotBothFriend")];
			m_pMgr->SendEvent(&chatInfo);
		}
		break;

	case E_Gift_Friend_Not_Online:
		{
			tagSocialInfoEvent chatInfo(_T("tagSocialInfoEvent"), NULL);
			chatInfo.strInfo = g_StrTable[_T("Social_ServerError_Outline")];
			m_pMgr->SendEvent(&chatInfo);
		}
		break;

	case E_Gift_Not_Gift:
		{
			tagSocialInfoEvent chatInfo(_T("tagSocialInfoEvent"), NULL);
			chatInfo.strInfo = g_StrTable[_T("Social_ServerError_NotGift")];
			m_pMgr->SendEvent(&chatInfo);
		}
		break;
	}

	return 0;
}

//-----------------------------------------------------------------------------
// 发送将roleID移动到好友列表group中的消息
//-----------------------------------------------------------------------------
VOID SocialMgr::SendMoveFriend( DWORD roleID, BYTE group )
{
	tagNC_UpdateFriGroup msg;
	msg.dwDestRoleID	= roleID;
	msg.byGroup			= group;
	m_pZoneSession->Send(&msg);
}

//-----------------------------------------------------------------------------
// 发送增加好友roleID的消息
//-----------------------------------------------------------------------------
VOID SocialMgr::SendAddFriend( DWORD roleID )
{
	tagNC_RoleMakeFriend msg;
	msg.dwDestRoleID	= roleID;
	m_pZoneSession->Send(&msg);
}

//-----------------------------------------------------------------------------
// 发送增加黑名单roleID的消息
//-----------------------------------------------------------------------------
VOID SocialMgr::SendAddBlack( DWORD roleID )
{
	tagNC_MoveToBlackList msg;
	msg.dwDestRoleID	= roleID;
	m_pZoneSession->Send(&msg);
}

//-----------------------------------------------------------------------------
// 发送删除好友roleID的消息
//-----------------------------------------------------------------------------
VOID SocialMgr::SendDeleteFriend( DWORD roleID )
{
	tagNC_RoleCancelFriend msg;
	msg.dwDestRoleID	= roleID;
	m_pZoneSession->Send(&msg);
}

//-----------------------------------------------------------------------------
// 发送删除黑名单的消息
//-----------------------------------------------------------------------------
VOID SocialMgr::SendDeleteBlack( DWORD roleID )
{
	tagNC_DeleteBlackList msg;
	msg.dwDestRoleID	= roleID;
	m_pZoneSession->Send(&msg);
}

//-----------------------------------------------------------------------------
// 发送删除仇敌名单的消息
//-----------------------------------------------------------------------------
VOID SocialMgr::SendDeleteFoe( DWORD roleID )
{
	tagNC_DeleteEnemy msg;
	msg.dwEnemyID	= roleID;
	m_pZoneSession->Send(&msg);
}

//-----------------------------------------------------------------------------
// 发送接收礼物的消息
//-----------------------------------------------------------------------------
VOID SocialMgr::SendGiftReply( DWORD giftID,BOOL bReceive )
{
	tagNC_SendGiftReply msg;
	msg.dwSrcRoleID		= m_sendRoleID;
	msg.dwTypeID		= giftID;
	msg.bResult			= bReceive;
	m_pZoneSession->Send(&msg);
}

//-----------------------------------------------------------------------------
// 发送给礼物的消息
//-----------------------------------------------------------------------------
VOID SocialMgr::SendGiveGiftTo( DWORD roleID,INT64 giftID )
{
	tagNC_SendGift msg;
	msg.dwDestRoleID	= roleID;
	msg.n64ItemID		= giftID;
	m_pZoneSession->Send(&msg);
}

//-----------------------------------------------------------------------------
// 发送接受骑乘邀请的消息
//-----------------------------------------------------------------------------
VOID SocialMgr::SendMountReply( BOOL bReceive )
{
	tagNC_MountOnInvite msg;
	msg.dwSrcRoleID		= m_mountRoleID;
	msg.bReceive		= bReceive;
	m_pZoneSession->Send(&msg);
}

//-----------------------------------------------------------------------------
// 打开好友列表
//-----------------------------------------------------------------------------
DWORD SocialMgr::OnEvent_OpenList( tagGameEvent* pEvent )
{
	//发送更新好友信息的消息
	tagNC_UpdateFriEnemyState msg;
	m_pZoneSession->Send(&msg);

	if( !P_VALID(m_pListFrame) )
	{
		DWORD dwParam = 0;
		if( m_showOutline )
			dwParam |= 0x00010000;
		if( m_autoReply )
			dwParam |= 0x00000001;
		m_pListFrame = (SocialListFrame*)m_pMgr->CreateFrame( m_strName.c_str(), _T("SocialList"), _T("SocialListFrame"), dwParam );
	}

	m_pListFrame->Show();

	return 0;
}

//-----------------------------------------------------------------------------
// 关闭好友列表
//-----------------------------------------------------------------------------
DWORD SocialMgr::OnEvent_CloseList( tagGameEvent* pEvent )
{
	if( P_VALID(m_pListFrame) )
	{
		m_pListFrame->Hide();
		m_pMgr->AddToDestroyList(m_pListFrame);
		m_pListFrame = NULL;
	}

	return 0;
}

//-----------------------------------------------------------------------------
// 增加好友
//-----------------------------------------------------------------------------
DWORD SocialMgr::OnEvent_AddFriend( tagAddFriendEvent* pEvent )
{
	AddFriend( pEvent->roleID );

	return 0;
}

//-----------------------------------------------------------------------------
// 增加黑名单
//-----------------------------------------------------------------------------
DWORD SocialMgr::OnEvent_AddBlack( tagAddBlackEvent* pEvent )
{
	AddBlack( pEvent->roleID );

	return 0;
}

//-----------------------------------------------------------------------------
// 使用礼物
//-----------------------------------------------------------------------------
DWORD SocialMgr::OnEvent_UseGift( tagUseGiftEvent* pEvent )
{
	m_giftGlobalID = pEvent->giftGlobalID;
	m_giftTypeID = pEvent->giftTypeID;

	if( !m_isUsingGift )
	{
		m_pMsgInputBox->Init( g_StrTable[_T("Social_GiveGiftCaption")], g_StrTable[_T("Social_GiveGiftText")], _T("SocialEvent_GiveGiftTo"), GT_INVALID, MBIT_Name, _T(""), FALSE);
		m_isUsingGift = true;
	}

	return 0;
}

//-----------------------------------------------------------------------------
// 把礼物给。。。
//-----------------------------------------------------------------------------
DWORD SocialMgr::OnEvent_GiveGiftTo( tagMsgInputBoxEvent* pEvent )
{
	m_isUsingGift = false;

	if(MBF_OK == pEvent->eResult)
	{
		DWORD roleID = PlayerNameTab::Inst()->FindIDByName( pEvent->strInputName.c_str() );
		if( P_VALID( roleID ) )
			GiveGiftTo( roleID );
		else
			m_giveGiftDelayIDs.push_back( pEvent->strInputName );
	}

	return 0;
}

//-----------------------------------------------------------------------------
// 获得礼物
//-----------------------------------------------------------------------------
DWORD SocialMgr::OnEvent_GetGift( tagMsgBoxEvent* pEvent )
{
	m_giftTiming = false;

	if( MBF_OK == pEvent->eResult )
		SendGiftReply( m_giftTypeID, TRUE );
	else
		SendGiftReply( m_giftTypeID, FALSE );

	return 0;
}

//-----------------------------------------------------------------------------
// 延迟获得某角色名称
//-----------------------------------------------------------------------------
DWORD SocialMgr::OnEvent_GetRoleName( tagRoleGetNameEvent* pEvent )
{
	DelayName::iterator nameIter;
	DelayBC::iterator bcIter;
	DelayID::iterator idIter;
	GetGiftName::iterator giftIter;
	GetInviteName::iterator invIter;

	// 好友登入
	for( nameIter=m_logInDelayNames.begin(); nameIter!=m_logInDelayNames.end(); )
	{
		if( *nameIter == pEvent->dwRoleID )//找到有相同ID的名字
		{
			ShowFriendLogInInfo( pEvent->dwRoleID, pEvent->strRoleName.c_str() );

			nameIter = m_logInDelayNames.erase( nameIter );
		}
		else
			++nameIter;
	}

	// 好友登出
	for( nameIter=m_logOutDelayNames.begin(); nameIter!=m_logOutDelayNames.end(); )
	{
		if( *nameIter == pEvent->dwRoleID )//找到有相同ID的名字
		{
			ShowFriendLogOutInfo( pEvent->dwRoleID, pEvent->strRoleName.c_str() );

			nameIter = m_logOutDelayNames.erase( nameIter );
		}
		else
			++nameIter;
	}

	//礼物广播
	for( bcIter=m_giftDelayBCs.begin(); bcIter!=m_giftDelayBCs.end(); )
	{
		tagBC& bc = *bcIter;
		if( bc.srcRoleID == pEvent->dwRoleID )//找到相同名字
		{
			bc.srcRoleID = GT_INVALID;
		}
		if( bc.destRoleID == pEvent->dwRoleID )//找到相同名字
		{
			bc.destRoleID = GT_INVALID;
		}

		if( (bc.srcRoleID == GT_INVALID) && (bc.destRoleID == GT_INVALID) )
		{
			ShowGiftBroadcast( &(bc.msg) );

			bcIter = m_giftDelayBCs.erase( bcIter );
		}
		else
			++bcIter;
	}

	//好友提醒
	for( nameIter=m_noticeDelayNames.begin(); nameIter!=m_noticeDelayNames.end(); )
	{
		if( *nameIter == pEvent->dwRoleID )//找到相同ID的名字
		{
			ShowFriendNotice( pEvent->strRoleName.c_str() );

			nameIter = m_noticeDelayNames.erase( nameIter );
		}
		else
			++nameIter;
	}

	//送礼物给
	for( idIter=m_giveGiftDelayIDs.begin(); idIter!=m_giveGiftDelayIDs.end(); )
	{
		if( *idIter == pEvent->strRoleName )//找到相同名字
		{
			GiveGiftTo( pEvent->dwRoleID );

			idIter = m_giveGiftDelayIDs.erase( idIter );
		}
		else
			++idIter;
	}

	//接收礼物
	for( giftIter=m_getGiftDelayNames.begin(); giftIter!=m_getGiftDelayNames.end(); )
	{
		tagNS_SendGiftToFri& msg = *giftIter;
		if( msg.dwSrcRoleID == pEvent->dwRoleID )//找到相同名字
		{
			ShowGetGift( &msg );

			giftIter = m_getGiftDelayNames.erase( giftIter );
		}
		else
			++giftIter;
	}

	//邀请骑乘
	for( invIter=m_getMountDelayNames.begin(); invIter!=m_getMountDelayNames.end(); )
	{
		tagNS_MountOnInvite& msg = *invIter;
		if( msg.dwSrcRoleID == pEvent->dwRoleID )//找到相同名字
		{
			ShowMountInvite( &msg );

			invIter = m_getMountDelayNames.erase( invIter );
		}
		else
			++invIter;
	}

	return 0;
}

DWORD SocialMgr::OnEvent_MsgAddBlack( tagMsgBoxEvent* pEvent )
{
	if(MBF_OK == pEvent->eResult)
		SendAddBlack( m_blackID );

	return 0;
}

DWORD SocialMgr::OnEvent_IsAddFriendMsgBox( tagMsgBoxEvent* pEvent )
{
	if( MBF_OK == pEvent->eResult )    //同意的话应该是进行好友添加
	{

		tagAddFriendEvent event(_T("SocialEvent_AddFriend"), this);
		event.roleID = m_addfriendRoleID;
		m_pMgr->SendEvent(&event);
	}
	return 0;
}

VOID SocialMgr::SetReply( LPCTSTR szReply )
{
	if( P_VALID(m_pReply) )
		m_pReply->SetReply( szReply );
}

LPCTSTR SocialMgr::GetReply( void ) const
{
	if( P_VALID(m_pReply) )
		return m_pReply->GetReply();

	return NULL;
}

//-----------------------------------------------------------------------------
// 好友登录后的一些界面操作
//-----------------------------------------------------------------------------
VOID SocialMgr::ShowFriendLogInInfo( DWORD roleID, LPCTSTR szRoleName )
{
	SocialData::EListType listType = m_pData->FriendLogIn( roleID );

	TCHAR szInfo[128];
	_stprintf( szInfo, g_StrTable[_T("Social_FriendLogIn")], szRoleName );
	tagSocialInfoEvent chatInfo(_T("tagSocialInfoEvent"), NULL);
	chatInfo.strInfo = szInfo;
	m_pMgr->SendEvent(&chatInfo);

	// 刷新好友列表显示
	if( P_VALID(m_pListFrame) )
		m_pListFrame->RefreshList( listType );
}

//-----------------------------------------------------------------------------
// 好友登出后的一些界面操作
//-----------------------------------------------------------------------------
VOID SocialMgr::ShowFriendLogOutInfo( DWORD roleID, LPCTSTR szRoleName )
{
	SocialData::EListType listType = m_pData->FriendLogOut( roleID );

	TCHAR szInfo[128];
	_stprintf( szInfo, g_StrTable[_T("Social_FriendLogOut")], szRoleName );
	tagSocialInfoEvent chatInfo(_T("tagSocialInfoEvent"), NULL);
	chatInfo.strInfo = szInfo;
	m_pMgr->SendEvent(&chatInfo);

	// 刷新好友列表显示
	if( P_VALID(m_pListFrame) )
		m_pListFrame->RefreshList( listType );
}

VOID SocialMgr::ShowGiftBroadcast( tagNS_SendGiftBroadcast* pMsg )
{
	LPCTSTR srcName = PlayerNameTab::Inst()->FindNameByID( pMsg->dwSrcRoleID );
	LPCTSTR destName = PlayerNameTab::Inst()->FindNameByID( pMsg->dwDestRoleID );

	const tagItemProto *pItem = ItemProtoData::Inst()->FindItemProto( pMsg->dwTypeID );
	if( !P_VALID(pItem) )
		return;

	ChatFrame* pFrame = (ChatFrame*)m_pFrameMgr->GetFrame(_T("Chat"));
	if( !P_VALID(pFrame) )
		return;

	TCHAR szContent[255];
	if( pMsg->bResult )
		_stprintf_s(szContent, 255, g_StrTable[_T("Social_ReceiveGift")], srcName, destName, pItem->pDisplayInfo->szName);
	else
		_stprintf_s(szContent, 255, g_StrTable[_T("Social_RefuseGift")], srcName, destName, pItem->pDisplayInfo->szName, destName, srcName);

	//pFrame->PushScrollText(szContent);
	pFrame->PushInfo(szContent, ESCC_Affiche);
}

VOID SocialMgr::ShowFriendNotice( LPCTSTR szRoleName )
{
	TCHAR szInfo[128];
	_stprintf( szInfo, g_StrTable[_T("Social_FriendNotice")], szRoleName );
	tagSocialInfoEvent chatInfo(_T("tagSocialInfoEvent"), NULL);
	chatInfo.strInfo = szInfo;
	m_pMgr->SendEvent(&chatInfo);
	m_addfriendRoleID = PlayerNameTab::Inst()->FindIDByName( szRoleName );               //获取角色ID
																						 //对方将你加为好友后弹出提示是否加对方为好友
	if( !m_pData->HasFriend( m_addfriendRoleID ) )                                       //如果对方不是好友添加对方为好友
	{
		ShowIsAddFriendMsgBox(szRoleName);                                               //显示消息框
	}
	
}

//----------------------------------------------------------------------------
//显示MsgBox
//----------------------------------------------------------------------------
VOID SocialMgr::ShowIsAddFriendMsgBox(LPCTSTR szRoleName)
{
	KillObjects();

	CreateObj("IsAddFriendMsgBox", "MsgBox");

	TCHAR msg[X_LONG_NAME] = {0};
	_stprintf( msg, g_StrTable[_T("Social_IsAddFriend")], szRoleName, szRoleName );       //szRoleName需要传进来，此值表示的是对方的名字

	TObjRef<MsgBox>("IsAddFriendMsgBox")->Init( _T(""), msg,
		_T("Socialevent_IsAddFriendMsgBox"), (MsgBoxFlag)(MBF_OK|MBF_Cancel), TRUE);

}

VOID SocialMgr::GiveGiftTo( DWORD roleID )
{
	if( P_VALID(roleID) )//输入的角色是否存在
	{
		tagFriendInfo* pInfo = m_pData->GetFriend( roleID );
		if( P_VALID(pInfo) )//好友存在
		{
			if( pInfo->bOnline )//好友在线
			{
				SendGiveGiftTo(roleID, m_giftGlobalID);
			}
			else
			{
				tagSocialInfoEvent chatInfo(_T("tagSocialInfoEvent"), NULL);
				chatInfo.strInfo = g_StrTable[_T("Social_GiveGift_Outline")];
				m_pMgr->SendEvent(&chatInfo);
			}
		}
		else
		{
			tagSocialInfoEvent chatInfo(_T("tagSocialInfoEvent"), NULL);
			chatInfo.strInfo = g_StrTable[_T("Social_GiveGift_NameIsNotExist")];
			m_pMgr->SendEvent(&chatInfo);
		}
	}
	else
	{
		tagSocialInfoEvent chatInfo(_T("tagSocialInfoEvent"), NULL);
		chatInfo.strInfo = g_StrTable[_T("Social_RoleIsNotExist")];
		m_pMgr->SendEvent(&chatInfo);
	}
}

VOID SocialMgr::ShowGetGift( tagNS_SendGiftToFri* pMsg )
{
	const tagItemProto* pItem = ItemProtoData::Inst()->FindItemProto( pMsg->dwTypeID );
	if( !P_VALID(pItem) )
		return;

	KillObjects();

	CreateObj("GiftChoiceBox", "MsgBox");

	TCHAR szText[128]={0};
	_stprintf( szText, g_StrTable[_T("Social_GetGiftText")], 
		PlayerNameTab::Inst()->FindNameByID(pMsg->dwSrcRoleID),
		pItem->pDisplayInfo->szName);
	m_sendRoleID = pMsg->dwSrcRoleID;
	m_giftTypeID = pMsg->dwTypeID;

	TObjRef<MsgBox>("GiftChoiceBox")->Init( 
		g_StrTable[_T("Social_GetGiftCaption")],
		szText,
		_T("SocialEvent_GetGift"),
		(MsgBoxFlag)(MBF_OK|MBF_Cancel) );

	// 开始计时
	m_giftTiming = true;
	m_timeSelStart = Kernel::Inst()->GetAccumTime();
}

VOID SocialMgr::AddFriend( DWORD roleID )
{
	if( m_pData->HasFriend( roleID ) )//已经是好友
	{
		tagSocialInfoEvent chatInfo(_T("tagSocialInfoEvent"), NULL);
		chatInfo.strInfo = g_StrTable[_T("Social_FriendIsExist")];
		m_pMgr->SendEvent(&chatInfo);
		return;
	}

	if( m_pData->GetFriendNum() >= MAX_FRIENDNUM )//好友列表已满
	{
		tagSocialInfoEvent chatInfo(_T("tagSocialInfoEvent"), NULL);
		chatInfo.strInfo = g_StrTable[_T("Social_FriendListIsFull")];
		m_pMgr->SendEvent(&chatInfo);
		return;
	}

	if( m_pData->HasBlack( roleID ) )//黑名单的无法被添加为好友
	{
		tagSocialInfoEvent chatInfo(_T("tagSocialInfoEvent"), NULL);
		chatInfo.strInfo = g_StrTable[_T("Social_BlackNotBeFriend")];
		m_pMgr->SendEvent(&chatInfo);
		return;
	}

	SendAddFriend( roleID );
}

VOID SocialMgr::AddBlack( DWORD roleID )
{
	if( m_pData->HasBlack( roleID ) )//已经在黑名单中
	{
		tagSocialInfoEvent chatInfo(_T("tagSocialInfoEvent"), NULL);
		chatInfo.strInfo = g_StrTable[_T("Social_BlackIsExist")];
		m_pMgr->SendEvent(&chatInfo);
		return;
	}

	if( m_pData->GetBlackNum() >= MAX_BLACKLIST )//黑名单已满
	{
		tagSocialInfoEvent chatInfo(_T("tagSocialInfoEvent"), NULL);
		chatInfo.strInfo = g_StrTable[_T("Social_BlackListIsFull")];
		m_pMgr->SendEvent(&chatInfo);
		return;
	}

	if( m_pData->HasFriend( roleID ) )//是好友
	{
		// 提示好友友好度将被清空
		m_blackID = roleID;
		TCHAR szText[128]={0};
		_stprintf( szText, g_StrTable[_T("Social_MoveToBlackText")], PlayerNameTab::Inst()->FindNameByID(roleID) );
		//KillObjects();
		if( P_VALID(GetObj("Social_MoveToBlack")) )
			KillObj("Social_MoveToBlack");
		CreateObj("Social_MoveToBlack", "MsgBox");
		TObjRef<MsgBox>("Social_MoveToBlack")->Init( 
			g_StrTable[_T("Social_MoveToBlackCaption")],
			szText,
			_T("SocialEvent_MsgAddBlack"),
			(MsgBoxFlag)(MBF_OK|MBF_Cancel),
			TRUE);
		return;
	}

	// todo 特殊关系的不可屏蔽，夫妻、师徒、情侣

	SendAddBlack( roleID );
}

DWORD SocialMgr::NetRecvMessage_MountOnInvite( tagNS_MountOnInvite* pMsg, DWORD pParam )
{
	m_mountList.push_back( *pMsg );

	return 0;
}

DWORD SocialMgr::OnEvent_Mount( tagMsgBoxEvent* pEvent )
{
	m_inviteTiming = false;

	if( MBF_OK == pEvent->eResult )
		SendMountReply( TRUE );
	else
		SendMountReply( FALSE );

	return 0;
}

DWORD SocialMgr::OnEvent_LPAttIsOK( tagGameEvent* pEvent )
{
	if( P_VALID(m_pReply) )
		SAFE_DELETE(m_pReply);

	m_pReply = new SocialReply();
	if( P_VALID(m_pReply) )
	{
		LocalPlayer* pPlayer = RoleMgr::Inst()->GetLocalPlayer();
		tstring strLocalName = pPlayer->GetRoleName();
		tstring strPath = _T("config\\");
		strPath += strLocalName;
		strPath += _T("\\");

		CreateDir( strPath.c_str() );
		strPath += _T("Reply.cfg");

		m_pReply->Init( strPath.c_str() );
	}

	return 0;
}

VOID SocialMgr::ShowMountInvite( tagNS_MountOnInvite* pMsg )
{
	KillObjects();
	CreateObj("MountInviteBox", "MsgBox");

	TCHAR szText[128]={0};
	_stprintf( szText, g_StrTable[_T("Mount_Invite")], 
		PlayerNameTab::Inst()->FindNameByID(pMsg->dwSrcRoleID) );
	m_mountRoleID = pMsg->dwSrcRoleID;

	TObjRef<MsgBox>("MountInviteBox")->Init( 
		_T(""),
		szText,
		_T("MountEvent_Invite"),
		(MsgBoxFlag)(MBF_OK|MBF_Cancel) );

	// 开始计时
	m_inviteTiming = true;
	m_timeInvStart = Kernel::Inst()->GetAccumTime();
}

DWORD SocialMgr::OnEvent_GetRoleIDError( tagGameEvent* pEvent )
{
	tagSocialInfoEvent chatInfo(_T("tagSocialInfoEvent"), NULL);
	chatInfo.strInfo = g_StrTable[_T("Social_RoleIsNotExist")];
	m_pMgr->SendEvent(&chatInfo);

	return 0;
}

DWORD SocialMgr::OnEvent_BloodBrother(tagBloodBrotherEvent* pEvent)
{
	if(!P_VALID(pEvent))	return 0;
	if(0 == pEvent->dwDays) return 0;
	tagNC_TryMakeBrother cmd;
	cmd.dwDays = pEvent->dwDays;
	m_pZoneSession->Send(&cmd);
	return 0;
}

DWORD SocialMgr::NetRecvMessage_InsertEnemy(tagNS_InsertEnemy* pMsg, DWORD pParam)
{
	DWORD	dwErrCode = pMsg->dwErrorCode;
	tagEnemyInfo& enemyInfo = pMsg->dwEnemyID;		//要添加的仇敌的角色ID

	switch( dwErrCode )
	{
	case E_Success:
		{
			bool isInFoeArray = FALSE; //默认名单里面有此人
			tagEnemyInfo& enemyInfo = pMsg->dwEnemyID;
			for ( INT j=0; j < (INT)m_pData->m_foeArray.size();j++)
			{
				tagEnemyInfo foeInfo = m_pData->m_foeArray[j];
				if ( foeInfo.dwEnemyID == enemyInfo.dwEnemyID )
				{
					isInFoeArray = TRUE;
					break;
				}
			}

			if( !P_VALID(enemyInfo.dwEnemyID) || isInFoeArray == TRUE )
				break;
			// 获得玩家名称
			PlayerNameTab::Inst()->FindNameByID( enemyInfo.dwEnemyID );
			if ( MAX_ENEMYNUM <= (INT)m_pData->m_foeArray.size() )
			{
				std::vector<tagEnemyInfo>::iterator iter = m_pData->m_foeArray.begin();
				m_pData->m_foeArray.erase(iter);
			}
			m_pData->m_foeArray.push_back(enemyInfo);
		}
		break;

	default:
		break;
	}

	return 0;
}

DWORD SocialMgr::NetRecvMessage_DeleteEnemy(tagNS_DeleteEnemy* pMsg, DWORD pParam)
{
	DWORD	dwErrCode = pMsg->dwErrorCode;
	DWORD	dwFoeID   = pMsg->dwEnemyID;		//要删除的仇敌的角色ID
	
	switch( dwErrCode )
	{
	case E_Success:
		{
			m_pData->DeleteFromFoeList( dwFoeID );
			
			// 刷新好友列表显示
			if( P_VALID(m_pListFrame) )
				m_pListFrame->RefreshList( SocialData::ELT_FoeList );
		}
		break;

	default:
		break;
	}
	return 0;

}

DWORD SocialMgr::NetRecvMessage_NS_EnemyLogin(tagNS_EnemyLogin* pMsg, DWORD pParam)
{
	DWORD	dwFoeID = pMsg->dwRoleID;

	const TCHAR* pTcharName = PlayerNameTab::Inst()->FindNameByID( dwFoeID );

	if (P_VALID(pTcharName))
	{

		TCHAR szText[512];
		_stprintf(szText, g_StrTable[_T("String_YourFoe_LoginIn")], pTcharName );
		CombatSysUtil::Inst()->ShowScreenCenterMsgEx(szText);
	}
	return 0;
}

VOID  SocialMgr::KillObjects()
{
	if(P_VALID(GetObj("Social_DeleteFriend")))
		KillObj("Social_DeleteFriend");
	if(P_VALID(GetObj("Social_AddFriend")))
		KillObj("Social_AddFriend");
	if(P_VALID(GetObj("Social_AddBlack")))
		KillObj("Social_AddBlack");

	if( P_VALID(GetObj("IsAddFriendMsgBox")) )
		KillObj("IsAddFriendMsgBox");
	if( P_VALID(GetObj("GiftChoiceBox")) )
		KillObj("GiftChoiceBox");
	if( P_VALID(GetObj("MountInviteBox")) )
		KillObj("MountInviteBox");

}

DWORD SocialMgr::NetRecvMessage_NS_TryMakeBrother(tagNS_TryMakeBrother* pMsg, DWORD pParam)
{
	switch (pMsg->dwErrorCode)
	{
	case E_bloodbrother_Success:
		{
			CombatSysUtil::Inst()->ShowScreenCenterMsg(g_StrTable[_T("E_bloodbrother_Success")]);
			LocalPlayer* plp = RoleMgr::Inst()->GetLocalPlayer();
			if (P_VALID(plp))
			{
				EffectMgr::Inst()->PlayRoleEffect( plp->GetID(), _T("Txxy09") );
				EffectMgr::Inst()->PlayRoleEffect( pMsg->dwRoleID, _T("Txxy09") );
			}
		}
		break;
	case E_bloodbrother_AlreadyHaveTeather:
		CombatSysUtil::Inst()->ShowScreenCenterMsg(g_StrTable[_T("E_bloodbrother_AlreadyHaveTeather")]);
		break;
	case E_bloodbrother_NotHaveItem:
		CombatSysUtil::Inst()->ShowScreenCenterMsg(g_StrTable[_T("E_bloodbrother_NotHaveItem")]);
		break;
	case E_bloodbrother_TeamNotHave2:
		CombatSysUtil::Inst()->ShowScreenCenterMsg(g_StrTable[_T("E_bloodbrother_TeamNotHave2")]);
		break;
	case E_bloodbrother_NotFriend:
		CombatSysUtil::Inst()->ShowScreenCenterMsg(g_StrTable[_T("E_bloodbrother_NotFriend")]);
		break;
	case E_bloodbrother_TeamLevelNotFeet:
		CombatSysUtil::Inst()->ShowScreenCenterMsg(g_StrTable[_T("E_bloodbrother_TeamLevelNotFeet")]);
		break;
	}
	return 0;
}

DWORD		 SocialMgr::NetRecvMessage_NS_BrotherLeftSecond(tagNS_BrotherLeftSecond* pMsg, DWORD pParam)
{
	tagBloodBrotherTime = pMsg->dwEndTime;
	m_dwTeacherID = pMsg->dwTeacherID;
	return 0;
}
DWORD		 SocialMgr::NetRecvMessage_NS_BrotherEnd(tagNS_BrotherEnd* pMsg, DWORD pParam)
{
	if ( 1 == pMsg->bReason)
	{
		if(P_VALID(GetObj("BloodBrotherEnd")))
			KillObj("BloodBrotherEnd");

		CreateObj("BloodBrotherEnd", "MsgBoxEx");
		TObjRef<MsgBoxEx>("BloodBrotherEnd")->Init( _T(""), g_StrTable[_T("BloodBrotherEnd_1")],NULL ,MBF_OK,FALSE);
	}
	else if (0 == pMsg->bReason)
	{
		if(P_VALID(GetObj("BloodBrotherEnd")))
			KillObj("BloodBrotherEnd");

		CreateObj("BloodBrotherEnd", "MsgBoxEx");
		TObjRef<MsgBoxEx>("BloodBrotherEnd")->Init( _T(""), g_StrTable[_T("BloodBrotherEnd_0")],NULL ,MBF_OK,FALSE);
	}

	m_dwTeacherID = GT_INVALID;
	tagBloodBrotherTime = 0;
	if(P_VALID(m_pListFrame))
		m_pListFrame->RefreshList();
	return 0;
}

INT			SocialMgr::GetBloodBrotherRemainDays()
{
	tagDWORDTime time = GetCurrentDWORDTime();
	DWORD LifeLastTime= CalcTimeDiff(tagBloodBrotherTime,time);
	LifeLastTime = LifeLastTime/3600;//转换为小时数
	INT day		=	LifeLastTime/24;
	INT hour	=	LifeLastTime%24;
	
	return day > 0 ? day+1 : 1;
}