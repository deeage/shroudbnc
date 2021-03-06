/*******************************************************************************
 * shroudBNC - an object-oriented framework for IRC                            *
 * Copyright (C) 2005-2007,2010 Gunnar Beutner                                 *
 *                                                                             *
 * This program is free software; you can redistribute it and/or               *
 * modify it under the terms of the GNU General Public License                 *
 * as published by the Free Software Foundation; either version 2              *
 * of the License, or (at your option) any later version.                      *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               *
 * GNU General Public License for more details.                                *
 *                                                                             *
 * You should have received a copy of the GNU General Public License           *
 * along with this program; if not, write to the Free Software                 *
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA. *
 *******************************************************************************/

#ifndef CHANNEL_H
#define CHANNEL_H

/**
 * chanmode_s
 *
 * A channel mode and its parameter.
 */
typedef struct chanmode_s {
	char Mode; /**< the channel mode */
	char *Parameter; /**< the associated parameter, or NULL if there is none */
} chanmode_t;

typedef struct backlog_s {
	time_t Time; /**< the time this message was received */
	char *Source; /**< message source, i.e. nick!ident@host */
	char *Message; /**< the message */
} backlog_t;

/* Forward declaration of some required classes */
class CNick;
class CBanlist;
class CIRCConnection;

#ifndef SWIG
int ChannelTSCompare(const void *p1, const void *p2);
int ChannelNameCompare(const void *p1, const void *p2);
#endif /* SWIG */

/**
 * CChannel
 *
 * Represents an IRC channel.
 */
class SBNCAPI CChannel : public CObject<CChannel, CIRCConnection> {
private:
	char *m_Name; /**< the name of the channel */
	time_t m_Creation; /**< the time when the channel was created */
	time_t m_Timestamp; /**< when the user joined the channel */

	CVector<chanmode_t> m_Modes; /**< the channel modes */
	bool m_ModesValid; /**< indicates whether the channelmodes are known */
	char *m_TempModes; /**< string-representation of the channel modes, used
							by GetChannelModes() */

	char *m_Topic; /**< the channel's topic */
	char *m_TopicNick; /**< the nick of the user who set the topic */
	time_t m_TopicStamp; /**< the time when the topic was set */
	int m_HasTopic; /**< indicates whether there is actually a topic */

	CHashtable<CNick *, false> m_Nicks; /**< a list of nicks who are on this channel */
	bool m_HasNames; /**< indicates whether m_Nicks is valid */
	bool m_KeepNicklist; /**< whether to keep the nicklist in memory */

	CBanlist *m_Banlist; /**< a list of bans for this channel */
	bool m_HasBans; /**< indicates whether the banlist is known */

	CList<backlog_t> m_Backlog; /** the backlog for this channel */
	int m_BacklogCount; /** the number of backlog lines we've stored for this channel */

	chanmode_t *AllocSlot(void);
	chanmode_t *FindSlot(char Mode);

public:
#ifndef SWIG
	CChannel(const char *Name, CIRCConnection *Owner);
	virtual ~CChannel(void);
#endif /* SWIG */

	const char *GetName(void) const;

	RESULT<const char *> GetChannelModes(void);
	void ParseModeChange(const char *source, const char *modes, int pargc, const char **pargv);

	time_t GetCreationTime(void) const;
	void SetCreationTime(time_t T);

	const char *GetTopic(void) const;
	void SetTopic(const char *Topic);

	const char *GetTopicNick(void) const;
	void SetTopicNick(const char *Nick);

	time_t GetTopicStamp(void) const;
	void SetTopicStamp(time_t TS);

	int HasTopic(void) const;
	void SetNoTopic(void);

	void AddUser(const char *Nick, const char *ModeChar);
	void RemoveUser(const char *Nick);
	void RenameUser(const char *Nick, const char *NewNick);

	bool HasNames(void) const;
	void SetHasNames(void);
	const CHashtable<CNick *, false> *GetNames(void) const;

	void ClearModes(void);
	bool AreModesValid(void) const;
	void SetModesValid(bool Valid);

	CBanlist *GetBanlist(void);
	void SetHasBans(void);
	bool HasBans(void) const;

	bool SendWhoReply(CClientConnection *Client, bool Simulate) const;

	time_t GetJoinTimestamp(void) const;

	void AddBacklogLine(const char *Source, const char *Message);
	void PlayBacklog(CClientConnection *Client);
};

#endif /* CHANNEL_H */
