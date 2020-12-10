#pragma once

#include <string>

class ExampleAppBase
{
public:
	ExampleAppBase(std::wstring appName, int width, int height, bool useRayTracing)
		: m_width(width)
		, m_height(height)
		, m_appName(appName)
		, m_useRayTracing(useRayTracing)
	{
	}

public:
	virtual bool Initialize() = 0;
	virtual void Update(float timeDelta) = 0;
	virtual void Render() = 0;
	virtual void Destroy() = 0;

public:

	int GetWidth() { return m_width; }
	int GetHeight() { return m_height; }

	std::wstring GetAppName() { return m_appName; }

	bool UseRayTracing() { return m_useRayTracing; }

public:
	int m_width;
	int m_height;

	std::wstring m_appName;

	bool m_useRayTracing = false;
};
