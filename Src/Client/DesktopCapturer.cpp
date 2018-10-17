#include "StdAfx.h"
#include "defaults.h"
#include "DesktopCapturer.h"

#include "webrtc/modules/desktop_capture/desktop_capture_options.h"
#include <iostream>

#pragma comment(lib,"desktop_capture.lib")
#pragma comment(lib,"desktop_capture_differ_sse2.lib")

// Delay between screen capture retries, in milliseconds
const int kCaptureDelay = 33;


namespace cricket
{
	DesktopCapturer::DesktopCapturer(cricket::ScreencastId id) 
		: screencastId_(id)
		, screen_capturer(nullptr)
		, running_(false)
		, initial_unix_timestamp_(time(NULL) * rtc::kNumNanosecsPerSec)
		, next_timestamp_(rtc::kNumNanosecsPerMillisec)
		, is_screencast_(false)
		, rotation_(webrtc::kVideoRotation_0)
	{
		// Default supported formats. Use ResetSupportedFormats to over write.
		std::vector<cricket::VideoFormat> formats;
// 		formats.push_back(cricket::VideoFormat(1280, 720,
// 			cricket::VideoFormat::FpsToInterval(30), cricket::FOURCC_I420));
// 		formats.push_back(cricket::VideoFormat(640, 480,
// 			cricket::VideoFormat::FpsToInterval(30), cricket::FOURCC_I420));
// 		formats.push_back(cricket::VideoFormat(320, 240,
// 			cricket::VideoFormat::FpsToInterval(30), cricket::FOURCC_I420));
// 		formats.push_back(cricket::VideoFormat(160, 120,
// 			cricket::VideoFormat::FpsToInterval(30), cricket::FOURCC_I420));

		formats.push_back(cricket::VideoFormat(1920, 1080,
			cricket::VideoFormat::FpsToInterval(30), cricket::FOURCC_I420));

		ResetSupportedFormats(formats);
	}

	DesktopCapturer::~DesktopCapturer()
	{
	}

	cricket::CaptureState DesktopCapturer::Start(const cricket::VideoFormat& format) 
	{
		cricket::VideoFormat supported;
		if (GetBestCaptureFormat(format, &supported)) {
			SetCaptureFormat(&supported);
		}
		running_ = true;
		SetCaptureState(cricket::CS_RUNNING);

		webrtc::DesktopCaptureOptions options;
		screen_capturer.reset(webrtc::ScreenCapturer::Create(options));
	
		webrtc::ScreenCapturer::ScreenList screenList;
		screen_capturer->GetScreenList(&screenList);

		for (auto it = screenList.begin(); it != screenList.end();++it)
		{
			if (it->id == screencastId_.desktop().index())
			{
				if (!screen_capturer->SelectScreen(it->id))
				{
					LOG(LS_ERROR) << "screen_capturer SelectScreen failed";
				}
				break;
			}
		}
		
		screen_capturer->Start(this);

		CaptureFrame();

		return cricket::CS_RUNNING;
	}

	void DesktopCapturer::Stop()
	{
		running_ = false;

		SetCaptureFormat(NULL);
		SetCaptureState(cricket::CS_STOPPED);
	}
	bool DesktopCapturer::IsRunning()
	{
		return running_;
	}

	void DesktopCapturer::SetScreencast(bool is_screencast)
	{
		is_screencast_ = is_screencast;
	}

	bool DesktopCapturer::IsScreencast() const 
	{ 
		return is_screencast_; 
	}

	bool DesktopCapturer::GetPreferredFourccs(std::vector<uint32>* fourccs) 
	{
		fourccs->push_back(cricket::FOURCC_I420);
		fourccs->push_back(cricket::FOURCC_MJPG);
		return true;
	}

	webrtc::SharedMemory* DesktopCapturer::CreateSharedMemory(size_t size)
	{
		return nullptr;
	}

	void DesktopCapturer::OnCaptureCompleted(webrtc::DesktopFrame* deskframe)
	{
		LOG(LS_INFO) << "DesktopCapturer Frame OnCaptureCompleted";

		uint32_t width = deskframe->size().width();
		uint32_t height = deskframe->size().height();
		uint32_t size = webrtc::DesktopFrame::kBytesPerPixel * width * height;

		cricket::CapturedFrame frame;
		frame.width = width;
		frame.height = height;
		frame.fourcc = cricket::FOURCC_ARGB;
		frame.data_size = size;
		frame.elapsed_time = next_timestamp_;
		frame.time_stamp = initial_unix_timestamp_ + next_timestamp_;
		next_timestamp_ += 33333333;  // 30 fps

		rtc::scoped_ptr<char[]> data(new char[size]);
		frame.data = data.get();
		memcpy(frame.data, deskframe->data(), size);
		frame.rotation = rotation_;

		// TODO(zhurunz): SignalFrameCaptured carry returned value to be able to
		// capture results from downstream.
		SignalFrameCaptured(this, &frame);

		delete deskframe;
	}

	void DesktopCapturer::OnMessage(rtc::Message* msg)
	{
		VideoCapturer::OnMessage(msg);

		if (msg->message_id == 0)
		{
			CaptureFrame();
		}
	}

	void DesktopCapturer::CaptureFrame()
	{
		LOG(LS_INFO) << "DesktopCapturer CaptureFrame";

		screen_capturer->Capture(webrtc::DesktopRegion());

		rtc::Thread::Current()->PostDelayed(kCaptureDelay, this, 0);
	}
}

