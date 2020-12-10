#include <glm/gtc/matrix_transform.hpp>
#include <math.h>
#include <algorithm>

#include "SphericalCoordinateMovementCamera.h"
#include "CoreEventManager.h"
#include "GlobalSystemValues.h"
#include "Utils.h"
#include "VulkanDeviceResources.h"


//matrix identity¸¸µéÀÚ

const float	SphericalCoordinateMovementCamera::DEFAULT_ROTATION_SENSITIVITY = 0.005f;
const float	SphericalCoordinateMovementCamera::DEFAULT_ZOOM_SENSITIVITY = 0.2f;

SphericalCoordinateMovementCamera::SphericalCoordinateMovementCamera()
	: m_width(0.0f)
	, m_height(0.0f)
	, m_near(0.0f)
	, m_far(0.0f)
	, m_fovAngleY(0.0f)
	, m_seta(0.0f)
	, m_phi(PI / 2.0f)
	, m_radaius(5.0f)
	, m_rotSensitivity(1.0f)
	, m_zoomSensitivity(1.0f)
	, m_needUpdateViewMatrix(true)
	, m_needUpdateProjectionMatrix(true)
	, m_useInputEvents(false)
{
	m_viewMatrix = glm::mat4(1.0f);
	m_projectionMatrix = glm::mat4(1.0f);
	m_viewProjMatrix = glm::mat4(1.0f);

	m_clipMatrix = glm::mat4(1.0f, 0.0f, 0.0f, 0.0f,
							 0.0f, -1.0f, 0.0f, 0.0f,
							 0.0f, 0.0f, 0.5f, 0.0f,
							 0.0f, 0.0f, 0.5f, 1.0f);

	memset(&m_mouseEventHandle, 0, sizeof(CoreEventHandle));
}

bool SphericalCoordinateMovementCamera::Initialize()
{
	RegistCallbackAndEvents();

	m_width = static_cast<float>(GlobalSystemValues::Instance().ScreenWidth);
	m_height = static_cast<float>(GlobalSystemValues::Instance().ScreenHeight);
	m_near = GlobalSystemValues::Instance().ViewportNearDistance;
	m_far = GlobalSystemValues::Instance().ViewportFarDistance;
	m_fovAngleY = GlobalSystemValues::Instance().FovAngleY;

	m_projectionMatrix = UpdateProjectionMatrix();

	return true;
}

void SphericalCoordinateMovementCamera::Destroy()
{
	UnregisterCallbackAndEvents();
}

bool SphericalCoordinateMovementCamera::Initialize(float width, float height, float nearDistance, float farDistance, float fovAngleY)
{
	RegistCallbackAndEvents();
	m_width = width;
	m_height = height;
	m_near = nearDistance;
	m_far = farDistance;
	m_fovAngleY = fovAngleY;
	m_projectionMatrix = UpdateProjectionMatrix();
	
	return true;
}

void SphericalCoordinateMovementCamera::RegistCallbackAndEvents()
{
	m_screenSizeChangedCallbackHandle = gVkDeviceRes.OnRenderTargetSizeChanged.Add
	(
		[this]()
		{
			m_width = static_cast<float>(gVkDeviceRes.GetWidth());
			m_height = static_cast<float>(gVkDeviceRes.GetHeight());
		}
	);
	m_mouseEventHandle = CoreEventManager::Instance().RegisterMouseEventCallback(this, &SphericalCoordinateMovementCamera::OnMouseEvent);
}

void SphericalCoordinateMovementCamera::UnregisterCallbackAndEvents()
{
	gVkDeviceRes.OnRenderTargetSizeChanged.Remove(m_screenSizeChangedCallbackHandle);
	CoreEventManager::Instance().UnregisterEventCallback(m_mouseEventHandle);
}

void SphericalCoordinateMovementCamera::OnMouseEvent(MouseEvent* mouseEvent)
{
	if (m_useInputEvents)
	{
		if (mouseEvent->CheckMouseEvent(EMouseEvent::MOUSE_MOVE))
		{
			m_seta += mouseEvent->m_dx * DEFAULT_ROTATION_SENSITIVITY * m_rotSensitivity;
			m_phi += mouseEvent->m_dy * DEFAULT_ROTATION_SENSITIVITY * m_rotSensitivity;
			m_needUpdateViewMatrix = true;
		}
		if (mouseEvent->CheckMouseEvent(EMouseEvent::MOUSE_WHEEL_UP))
		{
			m_radaius -= DEFAULT_ZOOM_SENSITIVITY * m_zoomSensitivity;
			m_radaius = max(m_radaius, 0.0f);
			m_needUpdateViewMatrix = true;
		}
		if (mouseEvent->CheckMouseEvent(EMouseEvent::MOUSE_WHEEL_DOWN))
		{
			m_radaius += DEFAULT_ZOOM_SENSITIVITY * m_zoomSensitivity;
			m_needUpdateViewMatrix = true;
		}
	}
}

void SphericalCoordinateMovementCamera::OnScreenSizeChanged(ScreenSizeChangedEvent* screenSizeChangedEvent)
{
	if (screenSizeChangedEvent != nullptr)
	{
		SetWidth(screenSizeChangedEvent->Width);
		SetHeight(screenSizeChangedEvent->Height);
	}
}

float SphericalCoordinateMovementCamera::GetFovAngleY()
{
	return m_fovAngleY;
}

void SphericalCoordinateMovementCamera::SetFovAngleY(float fovAngleY)
{
	m_needUpdateProjectionMatrix = true;
	m_fovAngleY = fovAngleY;
}

float SphericalCoordinateMovementCamera::GetWidth()
{
	return m_width;
}

void SphericalCoordinateMovementCamera::SetWidth(float width)
{
	m_needUpdateProjectionMatrix = true;
	m_width = width;
}

float SphericalCoordinateMovementCamera::GetHeight()
{
	return m_height;
}

void SphericalCoordinateMovementCamera::SetHeight(float height)
{
	m_needUpdateProjectionMatrix = true;
	m_height = height;
}

float SphericalCoordinateMovementCamera::GetNearDistance()
{
	m_needUpdateProjectionMatrix = true;
	return m_near;
}

void SphericalCoordinateMovementCamera::SetNearDistance(float nearDistance)
{
	m_near = nearDistance;
}

float SphericalCoordinateMovementCamera::GetFarDistance()
{
	m_needUpdateProjectionMatrix = true;
	return m_far;
}

void SphericalCoordinateMovementCamera::SetFarDistance(float farDistance)
{
	m_far = farDistance;
}

float SphericalCoordinateMovementCamera::GetRadius()
{
	return m_radaius;
}

void SphericalCoordinateMovementCamera::SetRadius(float radius)
{
	m_radaius = radius;
}

void SphericalCoordinateMovementCamera::SetSeta(float seta)
{
	m_seta = seta;
}

void SphericalCoordinateMovementCamera::SetPhi(float phi)
{
	m_phi = phi;
}

void SphericalCoordinateMovementCamera::SetRotSensitivity(float sensitivity)
{
	m_rotSensitivity = sensitivity;
}

void SphericalCoordinateMovementCamera::SetZoomSensitivity(float sensitivity)
{
	m_zoomSensitivity = sensitivity;
}

float SphericalCoordinateMovementCamera::GetAspectRatio()
{
	if (m_height > 0)
	{
		return m_width / m_height;
	}
	return -1.0f;
}

glm::vec3	SphericalCoordinateMovementCamera::GetPosition()
{
	return m_position;
}

void	SphericalCoordinateMovementCamera::UseInputEvents(bool use)
{
	m_useInputEvents = use;
}

glm::mat4 SphericalCoordinateMovementCamera::GetViewMatrix()
{
	if (m_needUpdateViewMatrix)
	{
		m_needUpdateViewMatrix = false;
		
		m_position = glm::vec3(m_radaius * sinf(m_phi) * cosf(m_seta),
							   m_radaius * cosf(m_phi),
							   m_radaius * sinf(m_phi) * sinf(m_seta));

		glm::vec3 lookAtPos(0.0f, 0.0f, 0.0f);
		glm::vec3 up(0.0f, -1.0f, 0.0f);
		m_viewMatrix = glm::lookAt(m_position, lookAtPos, up);
	}	
	return m_viewMatrix;
}

glm::mat4 SphericalCoordinateMovementCamera::GetProjectionMatrix()
{
	if (m_needUpdateProjectionMatrix)
	{
		m_needUpdateProjectionMatrix = false;
		m_projectionMatrix = UpdateProjectionMatrix();
	}
	return m_clipMatrix * m_projectionMatrix;
}

glm::mat4 SphericalCoordinateMovementCamera::GetViewProjectionMatrix()
{
	return GetProjectionMatrix() * GetViewMatrix();
}

glm::mat4 SphericalCoordinateMovementCamera::UpdateProjectionMatrix()
{
	return glm::perspective(m_fovAngleY, GetAspectRatio(), m_near, m_far);
}