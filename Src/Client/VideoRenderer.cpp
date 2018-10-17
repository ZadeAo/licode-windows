#include "stdafx.h"
#include "VideoRenderer.h"
#include "interface.h"


VideoRenderer::VideoRenderer(
	Renderer *render, int width, int height,
	webrtc::VideoTrackInterface* track_to_render)
	: renderer_(render), rendered_track_(track_to_render) {
	::InitializeCriticalSection(&buffer_lock_);
	ZeroMemory(&bmi_, sizeof(bmi_));
	bmi_.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi_.bmiHeader.biPlanes = 1;
	bmi_.bmiHeader.biBitCount = 32;
	bmi_.bmiHeader.biCompression = BI_RGB;
	bmi_.bmiHeader.biWidth = width;
	bmi_.bmiHeader.biHeight = -height;
	bmi_.bmiHeader.biSizeImage = width * height *
		(bmi_.bmiHeader.biBitCount >> 3);
	rendered_track_->AddRenderer(this);
}

VideoRenderer::~VideoRenderer() {
	rendered_track_->RemoveRenderer(this);
	::DeleteCriticalSection(&buffer_lock_);
}

void VideoRenderer::RegisterRenderer(Renderer *renderer)
{
	renderer_ = renderer;
}

void VideoRenderer::SetSize(int width, int height) {
	AutoLock<VideoRenderer> lock(this);

	bmi_.bmiHeader.biWidth = width;
	bmi_.bmiHeader.biHeight = -height;
	bmi_.bmiHeader.biSizeImage = width * height *
		(bmi_.bmiHeader.biBitCount >> 3);
	image_.reset(new uint8[bmi_.bmiHeader.biSizeImage]);
}

void VideoRenderer::RenderFrame(const cricket::VideoFrame* video_frame) {
	if (!video_frame)
		return;

	{
		AutoLock<VideoRenderer> lock(this);

		const cricket::VideoFrame* frame =
			video_frame->GetCopyWithRotationApplied();

		SetSize(static_cast<int>(frame->GetWidth()),
			static_cast<int>(frame->GetHeight()));

		ASSERT(image_.get() != NULL);
		frame->ConvertToRgbBuffer(cricket::FOURCC_ARGB,
			image_.get(),
			bmi_.bmiHeader.biSizeImage,
			bmi_.bmiHeader.biWidth *
			bmi_.bmiHeader.biBitCount / 8);
	}
	if (renderer_)
	{
		renderer_->RenderImage(bmi_, image());
	}
}