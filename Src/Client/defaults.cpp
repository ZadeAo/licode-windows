
#include "stdafx.h"
#include "defaults.h"

#include "setdebugnew.h"

const char kAudioLabel[] = "audio_label";
const char kVideoLabel[] = "video_label";
const char kDesktopLabel[] = "desktop_label";
const char kStreamLabel[] = "stream_label";

const CString WEBURL = _T("www.gupar.com");
const AString REG_URL = "www.gupar.com/member.php?mod=register";
const AString SERVER = "127.0.0.1";
const int		PORT = 6666;

// Names used for a IceCandidate JSON object.
const char kCandidateSdpMidName[] = "sdpMid";
const char kCandidateSdpMlineIndexName[] = "sdpMLineIndex";
const char kCandidateSdpName[] = "candidate";

// Names used for a SessionDescription JSON object.
const char kSessionDescriptionTypeName[] = "type";
const char kSessionDescriptionSdpName[] = "sdp";

AString GetEnvVarOrDefault(const char* env_var_name,
	const AString &default_value) {
	AString value;
	const char* env_var = getenv(env_var_name);
	if (env_var)
		value = env_var;

	if (value.empty())
		value = default_value;

	return value;
}

AString GetPeerName()
{
	AString ret(GetEnvVarOrDefault("USERNAME", "user"));
	return ret;
}

