#pragma once

#include "talk/app/webrtc/mediastreaminterface.h"
#include "talk/media/base/mediachannel.h"
#include "talk/media/base/videocommon.h"
#include "talk/media/base/videoframe.h"
#include "talk/media/base/videorenderer.h"

// A little helper class to make sure we always to proper locking and
// unlocking when working with VideoRenderer buffers.
template <typename T>
class AutoLock {
public:
	explicit AutoLock(T* obj) : obj_(obj) { obj_->Lock(); }
	~AutoLock() { obj_->Unlock(); }
protected:
	T* obj_;
};

class Renderer;

class VideoRenderer : public webrtc::VideoRendererInterface
{
public:
	VideoRenderer(Renderer *render, int width, int height, webrtc::VideoTrackInterface* track_to_render);
	virtual ~VideoRenderer();

	void Lock() {
		::EnterCriticalSection(&buffer_lock_);
	}

	void Unlock() {
		::LeaveCriticalSection(&buffer_lock_);
	}

	virtual void RegisterRenderer(Renderer *renderer);

	// VideoRendererInterface implementation
	virtual void SetSize(int width, int height);
	virtual void RenderFrame(const cricket::VideoFrame* frame);

	const BITMAPINFO& bmi() const { return bmi_; }
	const uint8* image() const { return image_.get(); }

protected:
	enum {
		SET_SIZE,
		RENDER_FRAME,
	};

	Renderer *renderer_;
	BITMAPINFO bmi_;
	rtc::scoped_ptr<uint8[]> image_;
	CRITICAL_SECTION buffer_lock_;
	rtc::scoped_refptr<webrtc::VideoTrackInterface> rendered_track_;
};

