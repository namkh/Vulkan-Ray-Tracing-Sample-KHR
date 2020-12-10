#pragma once

#include "Events.h"
#include "CoreEventManager.h"

#include <glm/glm.hpp>

class SphericalCoordinateMovementCamera
{
private:

	static const float	DEFAULT_ROTATION_SENSITIVITY;
	static const float	DEFAULT_ZOOM_SENSITIVITY;

public:

	SphericalCoordinateMovementCamera();

public:

	bool Initialize();
	bool Initialize(float width, float height, float nearDistance, float farDistance, float fovAngleY);
	void Destroy();

	void RegistCallbackAndEvents();
	void UnregisterCallbackAndEvents();

	void OnMouseEvent(MouseEvent* mouseEvent);
	void OnScreenSizeChanged(ScreenSizeChangedEvent* coreSystemEvent);

public:

	float	GetFovAngleY();
	void	SetFovAngleY(float fovAngleY);

	float	GetWidth();
	void	SetWidth(float width);

	float	GetHeight();
	void	SetHeight(float height);

	float	GetNearDistance();
	void	SetNearDistance(float nearDistance);
	float	GetFarDistance();
	void	SetFarDistance(float farDistance);

	float	GetRadius();
	void	SetRadius(float radius);

	void	SetSeta(float seta);
	void	SetPhi(float phi);

	void	SetRotSensitivity(float sensitivity);
	void	SetZoomSensitivity(float sensitivity);

	float	GetAspectRatio();

	glm::vec3	GetPosition();
	
	void	UseInputEvents(bool use);

public:

	glm::mat4 GetViewMatrix();
	glm::mat4 GetProjectionMatrix();
	glm::mat4 GetClipMatrix() { return m_clipMatrix; }
	glm::mat4 GetViewProjectionMatrix();

protected:

	glm::mat4 UpdateProjectionMatrix();

private:

	float	m_width = 0.0f;
	float	m_height = 0.0f;
	float	m_fovAngleY = 0.0f;
	float	m_near = 0.0f;
	float	m_far = 10000.0f;

	float	m_seta = 0.0f;
	float	m_phi = 0.0f;
	float	m_radaius = 10.0f;

	float	m_rotSensitivity = 1.0f;
	float	m_zoomSensitivity = 1.0f;

	//gl dx math wrapper구현해야할까... ;;
	glm::mat4	m_viewMatrix = glm::mat4(1.0f);
	glm::mat4	m_projectionMatrix = glm::mat4(1.0f);
	glm::mat4	m_clipMatrix = glm::mat4(1.0f);
	glm::mat4	m_viewProjMatrix = glm::mat4(1.0f);

	bool m_needUpdateViewMatrix = true;
	bool m_needUpdateProjectionMatrix = true;

	bool m_useInputEvents = false;

	glm::vec3 m_position = glm::vec3(0.0f);

private:

	CoreEventHandle m_mouseEventHandle = {};
	CommandHandle m_screenSizeChangedCallbackHandle = {};
};

