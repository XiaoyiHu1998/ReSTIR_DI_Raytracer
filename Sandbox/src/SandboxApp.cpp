#include <Hazel.h>

#include "imgui/imgui.h"
#include "imgui/misc/cpp/imgui_stdlib.h"
#include "glm/ext.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/string_cast.hpp"
#include "Glad/include/glad/glad.h"

#include "Include.h"
#include "RenderCommand.h"
#include "Renderer.h"
#include "Camera.h"
#include "AccelerationStructures.h"
#include "GeometryLoader.h"
#include "Utils.h"

class RaytracerLayer : public Hazel::Layer
{
public:
	RaytracerLayer() :
		Layer("PathTracing")
	{
		// Lights
		m_LightLocation = m_LightLocationSeed = 0;
		m_LightColor = m_LightColorSeed = 0;
		m_LightStrength = 0.65f;
		m_LightCount = 100;

		m_LightBoxSize = glm::vec3(50.0f, 7.0f, 9.0f);
		m_LightBoxPosition = glm::vec3(0.0f, 4.5f, 0.5f);
		GenerateLights();

		// Geometry
		m_TLAS;
		m_ObjectNames;
		m_AutoTransform;
		m_EnableAutoTransform;

		uint32_t sponzaIndex = LoadObject(".\\assets\\models\\sponza_small.obj", "Sponza");
		uint32_t sphereIndex = LoadObject(".\\assets\\models\\sphere_high_res.obj", "Sphere");
		uint32_t dragonIndex = LoadObject(".\\assets\\models\\dragon_460k.obj", "Dragon");
		uint32_t armadilloIndex = LoadObject(".\\assets\\models\\armadillo_small.obj", "Armadillo");

		m_TLAS.GetTransformRef(sponzaIndex) = Transform(glm::vec3(0), glm::vec3(0), glm::vec3(1));
		m_TLAS.GetTransformRef(sphereIndex) = Transform(glm::vec3(3.850f, 0.3f, 0.400f), glm::vec3(0, 0, 0), glm::vec3(1));
		m_TLAS.GetTransformRef(dragonIndex) = Transform(glm::vec3(12.350f, 0.850f, 0.650f), glm::vec3(0, 31.765f, 0), glm::vec3(1.5f));
		m_TLAS.GetTransformRef(armadilloIndex) = Transform(glm::vec3(5.550f, 0.0f, 2.650f), glm::vec3(0, 60.0f, 0), glm::vec3(1));
		m_TLAS.UpdateTransform();
		m_TLAS.Build();

		SetObjectAutoTransform(armadilloIndex, true, Transform(glm::vec3(0), glm::vec3(0.0f, 2.0f, 0.0f), glm::vec3(0)));
		SetObjectAutoTransform(dragonIndex, true, Transform(glm::vec3(0), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0)));

		// Setup Rendering
		HZ_INFO("Initiating Renderer");
		m_RendererSettingsUI;
		m_CurrentWidth = m_NextWidth = m_RendererSettingsUI.FrameWidth;
		m_CurrentHeight = m_NextHeight = m_RendererSettingsUI.FrameHeight;

		m_Camera = Camera(m_CurrentWidth, m_CurrentHeight, 60);
		m_Camera.position = glm::vec3(-0.195f, 2.07f, -0.195f);
		m_Camera.rotation = glm::vec3(8.144f, 111.0f, 0.0f);
		m_MoveCamera = false;

		m_CameraMoveSpeed = 0.25f;
		m_CameraRotationSpeed = 11.25f;

		m_CameraFlyMoveSpeed = glm::vec3(0.125f, 0.0f, 0.0f);
		m_CameraFlyRotationSpeed = glm::vec3(0.0f, 0.0f, 0.0f);

		m_Renderer;
		m_Renderer.Init(m_RendererSettingsUI, Renderer::Scene(m_Camera, m_TLAS, m_pointLights));

		FrameBufferRef frameBuffer = m_Renderer.GetFrameBuffer();
		RenderCommand::GeneratePixelBufferObject(m_PixelBufferObjectID, frameBuffer, m_CurrentWidth, m_CurrentHeight);
		RenderCommand::GenerateFrameBufferTexture(m_FrameBufferID, frameBuffer, m_CurrentWidth, m_CurrentHeight);
		RenderCommand::InitFrameBuffer(m_FrameBufferID, m_PixelBufferObjectID);
		RenderCommand::UploadFrameData(m_FrameBufferID, m_PixelBufferObjectID, frameBuffer, m_CurrentWidth, m_CurrentHeight);

		// UI
		m_SelectedNode = 0;
	}

	~RaytracerLayer()
	{
		m_Renderer.Terminate();
		RenderCommand::DeleteFrameBufferTexture(m_FrameBufferID);
	}

	void OnUpdate(Hazel::Timestep timestep) override
	{
		// Display rendererd frame
		FrameBufferRef frameBuffer = m_Renderer.GetFrameBuffer();
		glm::i32vec2 m_ViewportResolution = m_Renderer.GetRenderResolution();
		RenderCommand::UploadFrameData(m_FrameBufferID, m_PixelBufferObjectID, frameBuffer, m_ViewportResolution.x, m_ViewportResolution.y);

		// Setup next frame to be rendered
		m_CurrentWidth = m_NextWidth;
		m_CurrentHeight = m_NextHeight;

		m_RendererSettingsUI.FrameWidth = m_CurrentWidth;
		m_RendererSettingsUI.FrameHeight = m_CurrentHeight;

		// Camera Input
		HandleCameraControls(timestep);

		// Auto move camera
		if (m_MoveCamera)
		{
			m_Camera.position += m_CameraFlyMoveSpeed * timestep.GetTimeSeconds();
			m_Camera.rotation += m_CameraFlyRotationSpeed * timestep.GetTimeSeconds();
			//m_Camera.position += glm::vec3(0.125f * timestep, 0, 0);
		}

		// Rigid object animations
		for (int i = 0; i < m_TLAS.GetObjectCount(); i++)
		{
			if (m_EnableAutoTransform[i])
			{
				m_TLAS.GetTransformRef(i).translation += m_AutoTransform[i].translation * timestep.GetTimeSeconds();
				m_TLAS.GetTransformRef(i).rotation += m_AutoTransform[i].rotation * timestep.GetTimeSeconds();
				m_TLAS.GetTransformRef(i).scale += m_AutoTransform[i].scale * timestep.GetTimeSeconds();

				if (m_TLAS.GetTransformRef(i).scale.x == 0.0f)
					m_TLAS.GetTransformRef(i).scale.x = 0.0000001f;

				if (m_TLAS.GetTransformRef(i).scale.y == 0.0f)
					m_TLAS.GetTransformRef(i).scale.y = 0.0000001f;

				if (m_TLAS.GetTransformRef(i).scale.z == 0.0f)
					m_TLAS.GetTransformRef(i).scale.z = 0.0000001f;
			}
		}

		// Submit new scene
		m_Renderer.SubmitRenderSettings(m_RendererSettingsUI);
		m_Renderer.SubmitScene(Renderer::Scene(m_Camera, m_TLAS, m_pointLights));
	}

	virtual void OnImGuiRender()
	{
		DrawImGuiDockSpace();

		// Viewport Window
		{
			// Draw Viewport Texture
			ImGui::Begin("Viewport");
			ImVec2 nextFrameResolution = ImGui::GetContentRegionAvail();
			ImVec2 viewportPosition = ImGui::GetWindowPos();

			m_NextWidth = std::max((uint32_t)2, static_cast<uint32_t>(nextFrameResolution.x));
			m_NextHeight = std::max((uint32_t)2, static_cast<uint32_t>(nextFrameResolution.y));

			ImGui::Image((void*)(intptr_t)m_FrameBufferID, ImVec2(m_CurrentWidth, m_CurrentHeight));
			ImGui::End();

			// Performance Metrics Subwindow
			ImGui::SetNextWindowBgAlpha(0.45f);
			ImGui::SetNextWindowPos(ImVec2(viewportPosition.x + 16, viewportPosition.y + 36));
			ImGui::SetNextWindowSize(ImVec2(110, 55));
			ImGui::Begin("Performance Metrics", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
			ImGui::SetCursorPos(ImVec2(8, 8));
			ImGui::Text("%dx%d", (int)(m_CurrentWidth), (int)(m_CurrentHeight));
			ImGui::SetCursorPos(ImVec2(8, 20));
			ImGui::Text("%.1f FPS", 1000.0f / m_Renderer.GetLastFrameTime());
			ImGui::SetCursorPos(ImVec2(8, 32));
			ImGui::Text("%.3f ms", m_Renderer.GetLastFrameTime());
			ImGui::End();

			// Controls Subwindow
			ImGui::SetNextWindowBgAlpha(0.45f);
			ImGui::SetNextWindowPos(ImVec2(viewportPosition.x + 16, viewportPosition.y + nextFrameResolution.y - 60));
			ImGui::SetNextWindowSize(ImVec2(140, 80));
			ImGui::Begin("Movement Controls", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
			ImGui::SetCursorPos(ImVec2(8, 8));
			ImGui::Text("Q/E: Down/Up");
			ImGui::SetCursorPos(ImVec2(8, 20));
			ImGui::Text("WASD: Movement");
			ImGui::SetCursorPos(ImVec2(8, 32));
			ImGui::Text("R: Reset camera");
			ImGui::SetCursorPos(ImVec2(8, 44));
			ImGui::Text("Left Alt: Rotate");
			ImGui::SetCursorPos(ImVec2(8, 56));
			ImGui::Text("Left Shift: Sprint");
			ImGui::End();

			// Triangle Count Window
			ImGui::SetNextWindowBgAlpha(0.45f);
			ImGui::SetNextWindowPos(ImVec2(viewportPosition.x + 130, viewportPosition.y + 36));
			ImGui::SetNextWindowSize(ImVec2(160, 16));
			ImGui::Begin("Triangle Count", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
			ImGui::SetCursorPos(ImVec2(8, 8));
			ImGui::Text("Triangles:%d", m_TLAS.GetTriangleCount());
			ImGui::End();
		}

		// Settings Window
		{
			ImGui::Begin("Settings");
			ImGui::PushItemWidth(-ImGui::GetWindowWidth() * 0.35f);
			ImGui::Text("Render Settings");
			const char* RenderModes[] = { "Normals", "TraversalSteps", "Next Event Estimation DI", "ReSTIR DI" };
			int selectedMode = static_cast<int>(m_RendererSettingsUI.Mode);
			ImGui::Combo("Render Mode", &selectedMode, RenderModes, IM_ARRAYSIZE(RenderModes));
			m_RendererSettingsUI.Mode = static_cast<RendererSettings::RenderMode>(selectedMode);

			ImGui::PushItemWidth(-ImGui::GetWindowWidth() * 0.65f);
			if (ImGui::InputInt("Thread Count", &m_RendererSettingsUI.ThreadCount, 1, 1))
			{
				m_RendererSettingsUI.ThreadCount = std::max(0, m_RendererSettingsUI.ThreadCount);
				m_RendererSettingsUI.ThreadCount = std::min(static_cast<int>(std::thread::hardware_concurrency()), m_RendererSettingsUI.ThreadCount);
			}
			if (ImGui::InputInt("Tile size", &m_RendererSettingsUI.TileSize))
			{
				m_RendererSettingsUI.TileSize = std::min(std::max(4, m_RendererSettingsUI.TileSize), 256);
			}
			ImGui::DragFloat("Eta size", &m_RendererSettingsUI.Eta, 0.001f, 0.001f, 0.1f);
			ImGui::Checkbox("Random Seed", &m_RendererSettingsUI.RandomSeed);
			ImGui::Separator();

			if (m_RendererSettingsUI.Mode == RendererSettings::RenderMode::Normals)
			{
				ImGui::Text("Normal Rendering");
				ImGui::Checkbox("Render Previous Normals", &m_RendererSettingsUI.RenderPrevNormals);
				ImGui::Separator();
			}
			else if (m_RendererSettingsUI.Mode == RendererSettings::RenderMode::DI)
			{
				ImGui::Text("DI Rendering");
				ImGui::Checkbox("Sample All Lights", &m_RendererSettingsUI.SampleAllLightsDI);
				ImGui::Checkbox("Light Occlusion", &m_RendererSettingsUI.OcclusionCheckDI);
				if (ImGui::InputInt("Light Candidates", &m_RendererSettingsUI.CandidateCountDI))
				{
					m_RendererSettingsUI.CandidateCountDI = m_RendererSettingsUI.CandidateCountDI < 1 ? 1 : m_RendererSettingsUI.CandidateCountDI;
				}
				ImGui::Separator();
			}
			else if (m_RendererSettingsUI.Mode == RendererSettings::RenderMode::ReSTIR)
			{
				// Streaming RIS
				ImGui::Text("Streaming RIS");
				if (ImGui::InputInt("Candidate Count", &m_RendererSettingsUI.CandidateCountReSTIR))
				{
					m_RendererSettingsUI.CandidateCountReSTIR = m_RendererSettingsUI.CandidateCountReSTIR < 1 ? 1 : m_RendererSettingsUI.CandidateCountReSTIR;
				}
				ImGui::Separator();
				ImGui::Text("Visibility Pass");
				ImGui::Checkbox("Enable", &m_RendererSettingsUI.EnableVisibilityPass);
				ImGui::Separator();

				// Temporal Reuse
				ImGui::PushID("Temporal Reuse Options");
				ImGui::Text("Temporal Reuse");
				ImGui::Checkbox("Enable", &m_RendererSettingsUI.EnableTemporalReuse);
				if (ImGui::InputInt("Sample Count Ratio", &m_RendererSettingsUI.TemporalSampleCountRatio))
				{
					m_RendererSettingsUI.TemporalSampleCountRatio = m_RendererSettingsUI.TemporalSampleCountRatio < 1 ? 1 : m_RendererSettingsUI.TemporalSampleCountRatio;
				}
				ImGui::DragFloat("Max Distance", &m_RendererSettingsUI.TemporalMaxDistance, 0.001f, 0.0f, 1.0f);
				ImGui::DragFloat("Max Distance Depth Scaling", &m_RendererSettingsUI.TemporalMaxDistanceDepthScaling, 0.001f, 0.0f, 5.0f);
				ImGui::DragFloat("Min Normal Similarity", &m_RendererSettingsUI.TemporalMinNormalSimilarity, 0.001f, 0.0f, 1.0f);
				ImGui::Separator();
				ImGui::PopID();

				// Spatial Reuse
				ImGui::PushID("Spatial Reuse Options");
				ImGui::Text("Spatial Reuse");
				ImGui::Checkbox("Enable", &m_RendererSettingsUI.EnableSpatialReuse);
				if (ImGui::InputInt("Neighbours", &m_RendererSettingsUI.SpatialReuseNeighbours))
				{
					m_RendererSettingsUI.SpatialReuseNeighbours = m_RendererSettingsUI.SpatialReuseNeighbours < 1 ? 1 : m_RendererSettingsUI.SpatialReuseNeighbours;
					int maxNeighbours = std::max(m_RendererSettingsUI.SpatialPixelRadius - 5, 1);
					m_RendererSettingsUI.SpatialReuseNeighbours = m_RendererSettingsUI.SpatialReuseNeighbours > maxNeighbours ? maxNeighbours : m_RendererSettingsUI.SpatialReuseNeighbours;
				}
				if (ImGui::InputInt("Pixel Radius", &m_RendererSettingsUI.SpatialPixelRadius))
				{
					m_RendererSettingsUI.SpatialPixelRadius = m_RendererSettingsUI.SpatialPixelRadius < 3 ? 3 : m_RendererSettingsUI.SpatialPixelRadius;
				}
				ImGui::DragFloat("Max Distance", &m_RendererSettingsUI.SpatialMaxDistance, 0.001f, 0.0f, 1.0f);
				ImGui::DragFloat("Max Distance Depth Scaling", &m_RendererSettingsUI.SpatialMaxDistanceDepthScaling, 0.001f, 0.0f, 5.0f);
				ImGui::DragFloat("Min Normal Similarity", &m_RendererSettingsUI.SpatialMinNormalSimilarity, 0.001f, 0.0f, 1.0f);
				ImGui::Separator();
				ImGui::PopID();
			}

			ImGui::End();
		}

		// Outliner Window
		{
			ImGui::Begin("Outliner");

			// Configurate TreeNodeEX
			static ImGuiTreeNodeFlags baseNodeFlags = ImGuiTreeNodeFlags_OpenOnArrow |
													  ImGuiTreeNodeFlags_OpenOnDoubleClick |
													  ImGuiTreeNodeFlags_SpanAvailWidth |
													  ImGuiTreeNodeFlags_SpanFullWidth;

			// TreeNodeEX Drawing Lambda
			auto DrawImGUiTreeNodeEX = [&](uint32_t index, const char* nodeName)
			{
				ImGuiTreeNodeFlags currentNodeFlag = baseNodeFlags;
				currentNodeFlag |= ImGuiTreeNodeFlags_NoTreePushOnOpen;
				if (m_SelectedNode == index)
					currentNodeFlag |= ImGuiTreeNodeFlags_Selected;

				ImGui::TreeNodeEx((void*)(intptr_t)index, currentNodeFlag, nodeName, index);

				if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
					m_SelectedNode = index;
			};

			// Draw Scene
			DrawImGUiTreeNodeEX(0, "Camera");
			DrawImGUiTreeNodeEX(1, "Lights");
			for (uint32_t i = 0; i < m_TLAS.GetObjectCount(); i++)
			{
				DrawImGUiTreeNodeEX(i + 2, m_ObjectNames[i].c_str());
			}

			ImGui::End();
		}

		// Properties Window
		{
			ImGui::Begin("Properties");
			if (m_SelectedNode == 0)
			{
				bool cameraUpdated = false;

				ImGui::PushID("Properties_Camera");

				ImGui::Text("Camera");
				ImGui::Separator();

				ImGui::Text("Transform");
				ImGui::DragFloat3("Position", glm::value_ptr(m_Camera.position), 0.05f);
				ImGui::DragFloat3("Rotation", glm::value_ptr(m_Camera.rotation), 0.05f);
				ImGui::Separator();

				ImGui::Text("View");
				ImGui::PushItemWidth(-ImGui::GetWindowWidth() * 0.5f);
				ImGui::DragFloat("Vertical FoV", &m_Camera.verticalFOV, 0.1f, 1.0f, 160.0f);
				ImGui::Separator();

				ImGui::Text("Auto Fly");
				ImGui::Checkbox("Enable", &m_MoveCamera);
				ImGui::DragFloat3("Movement Speed", glm::value_ptr(m_CameraFlyMoveSpeed), 0.05f);
				ImGui::DragFloat3("Rotation Speed", glm::value_ptr(m_CameraFlyRotationSpeed), 0.05f);
				ImGui::Separator();

				ImGui::Text("Controls");
				ImGui::DragFloat("Movement Speed", &m_CameraMoveSpeed, 0.1f, 0.001f, 5.0f);
				ImGui::DragFloat("Rotation Speed", &m_CameraRotationSpeed, 0.1f, 0.001f, 5.0f);
				ImGui::Separator();

				ImGui::PopID();
			}
			else if (m_SelectedNode == 1)
			{
				ImGui::PushID("Properties_Generation");
				ImGui::Text("Light Generation");
				ImGui::Separator();
				ImGui::Text("Light Bounding Box");
				ImGui::DragFloat3("Size", glm::value_ptr(m_LightBoxSize), 0.05f);
				ImGui::DragFloat3("Position", glm::value_ptr(m_LightBoxPosition), 0.05f);
				ImGui::Separator();
				ImGui::Text("Light Properties");
				ImGui::Separator();
				ImGui::PushItemWidth(-ImGui::GetWindowWidth() * 0.75f);
				ImGui::DragInt("Count", &m_LightCount, 1, 0, 10000);
				ImGui::DragInt("Color Seed", &m_LightColor, 1, 0, 1000);
				ImGui::DragInt("Location Seed", &m_LightLocation, 1, 0, 1000);

				ImGui::DragFloat("Intensity", &m_LightStrength, 0.010f, 0.0f, 30.0f);

				if (ImGui::Button("Generate"))
				{
					m_LightColorSeed = static_cast<uint32_t>(m_LightColor);
					m_LightLocationSeed = static_cast<uint32_t>(m_LightLocation);

					GenerateLights();
					m_Renderer.InvalidateHistory();
				}

				ImGui::PopID();
			}
			else if (2 <= m_SelectedNode && m_SelectedNode < m_TLAS.GetObjectCount() + 2)
			{
				uint32_t blasIndex = m_SelectedNode - 2;
				bool transformUpdated = false;

				ImGui::PushID(char(blasIndex));
				ImGui::InputText("Name", &m_ObjectNames[blasIndex]);
				ImGui::Separator();

				ImGui::Text("Transform");
				ImGui::DragFloat3("Position", glm::value_ptr(m_TLAS.GetTransformRef(blasIndex).translation), 0.05f);
				ImGui::DragFloat3("Rotation", glm::value_ptr(m_TLAS.GetTransformRef(blasIndex).rotation), 0.05f);
				if (ImGui::DragFloat3("Scale", glm::value_ptr(m_TLAS.GetTransformRef(blasIndex).scale), 0.05f))
				{
					// Prevent crashing due to NaN rays
					m_TLAS.GetTransformRef(blasIndex).scale.x = std::max(0.00000001f, m_TLAS.GetTransformRef(blasIndex).scale.x);
					m_TLAS.GetTransformRef(blasIndex).scale.y = std::max(0.00000001f, m_TLAS.GetTransformRef(blasIndex).scale.y);
					m_TLAS.GetTransformRef(blasIndex).scale.z = std::max(0.00000001f, m_TLAS.GetTransformRef(blasIndex).scale.z);
				}
				ImGui::Separator();
				ImGui::Text("Animation");
				ImGui::Checkbox("Enable", (bool*)(&(m_EnableAutoTransform[blasIndex])));
				ImGui::DragFloat3("Translation Speed", glm::value_ptr(m_AutoTransform[blasIndex].translation), 0.05f);
				ImGui::DragFloat3("Rotation Speed", glm::value_ptr(m_AutoTransform[blasIndex].rotation), 0.05f);
				ImGui::DragFloat3("Scale Speed", glm::value_ptr(m_AutoTransform[blasIndex].scale), 0.05f);
				ImGui::Separator();

				ImGui::PopID();
			}
			ImGui::End();
		}

		RenderCommand::ClearFrame();
	}

	void OnEvent(Hazel::Event& event) override
	{

	}
private:
	// Viewport and rendering
	uint32_t m_FrameBufferID;
	uint32_t m_PixelBufferObjectID;
	uint32_t m_CurrentWidth, m_CurrentHeight;
	uint32_t m_NextWidth, m_NextHeight;
	glm::i32vec2 m_PrevFrameResolution;

	//Renderer
	Renderer m_Renderer;
	RendererSettings m_RendererSettingsUI;

	// Camera
	Camera m_Camera;
	float m_CameraMoveSpeed;
	float m_CameraRotationSpeed;

	glm::vec3 m_CameraFlyMoveSpeed;
	glm::vec3 m_CameraFlyRotationSpeed;

	// World state
	TLAS m_TLAS;
	std::vector<PointLight> m_pointLights;
	float m_LightStrength;
	int m_LightCount;
	int m_LightColor;
	int m_LightLocation;
	uint32_t m_LightColorSeed;
	uint32_t m_LightLocationSeed;

	glm::vec3 m_LightBoxSize;
	glm::vec3 m_LightBoxPosition;

	// Object Animation
	std::vector<uint32_t> m_EnableAutoTransform;
	std::vector<Transform> m_AutoTransform;
		
	// UI
	int m_SelectedNode;
	bool m_MoveCamera;
	std::vector<std::string> m_ObjectNames;
private:
	void DrawImGuiDockSpace()
	{
		static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
		static bool dockspaceOpen = true;

		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
		ImGuiViewport* viewport_ImGui = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport_ImGui->WorkPos);
		ImGui::SetNextWindowSize(viewport_ImGui->WorkSize);
		ImGui::SetNextWindowViewport(viewport_ImGui->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

		if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode) {
			window_flags |= ImGuiWindowFlags_NoBackground;
		}

		// Important: keep maindockspace always open
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("DockSpace Demo", &dockspaceOpen, window_flags);
		ImGui::PopStyleVar();
		ImGui::PopStyleVar(2);

		// Main dockspace, this is where all our windows are drawn
		ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("Files"))
			{
				if (ImGui::MenuItem("Import OBJ"))
				{
					std::string objFilePath;
					if (GetObjFilePath(objFilePath))
						LoadObject(objFilePath);
				}
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}
		ImGui::End();
	}

	void GenerateLights()
	{
		m_pointLights = std::vector<PointLight>();
		m_pointLights.reserve(m_LightCount);

		HZ_INFO("Generating {} Lights", m_LightCount);
		for (int i = 0; i < m_LightCount; i++)
		{
			// Position
			float x = (Utils::RandomFloat(m_LightLocationSeed) - 0.5f) * m_LightBoxSize.x + m_LightBoxPosition.x;
			float y = (Utils::RandomFloat(m_LightLocationSeed) - 0.5f) * m_LightBoxSize.y + m_LightBoxPosition.y;
			float z = (Utils::RandomFloat(m_LightLocationSeed) - 0.5f) * m_LightBoxSize.z + m_LightBoxPosition.z;
			glm::vec3 position = glm::vec3(x, y, z);

			// Color
			float r = std::max(0.2f, Utils::RandomFloat(m_LightColorSeed));
			float g = std::max(0.2f, Utils::RandomFloat(m_LightColorSeed));
			float b = std::max(0.2f, Utils::RandomFloat(m_LightColorSeed));
			glm::vec3 emissiveColor = glm::vec3(r, g, b);

			m_pointLights.emplace_back(position, emissiveColor, m_LightStrength);
		}
	}

	uint32_t LoadObject(const std::string& fileName, const std::string& objectName)
	{
		// Load object
		HZ_INFO("Loading Object {} from {}", objectName, fileName);
		std::vector<tinybvh::bvhvec4> vertexBuffer;
		GeometryLoader::LoadObj(fileName, vertexBuffer);

		std::shared_ptr<BLAS> blas = std::make_shared<BLAS>();
		Transform objectTransform;
		objectTransform.translation = m_Camera.position + m_Camera.Forward() * 3.5f;
		blas->SetObject(vertexBuffer);

		AddObjectToRotationList();
		m_ObjectNames.push_back(objectName);

		return m_TLAS.AddBLAS(blas, objectTransform);
	}

	uint32_t LoadObject(const std::string& fileName)
	{
		// Get objectName from fileName
		std::string objectName = fileName;
		std::string delimiter = "\\";
		auto position = fileName.find(delimiter);
		while (position != std::string::npos)
		{
			objectName.erase(0, position + delimiter.length());
			position = objectName.find(delimiter);
		}
		position = objectName.find(".");
		objectName = objectName.substr(0, position);

		// Load object
		HZ_INFO("Loading Object {} from {}", objectName, fileName);
		std::vector<tinybvh::bvhvec4> vertexBuffer;
		GeometryLoader::LoadObj(fileName, vertexBuffer);

		std::shared_ptr<BLAS> blas = std::make_shared<BLAS>();
		Transform objectTransform;
		objectTransform.translation = m_Camera.position + m_Camera.Forward() * 3.5f;
		blas->SetObject(vertexBuffer);

		AddObjectToRotationList();
		m_ObjectNames.push_back(objectName);

		return m_TLAS.AddBLAS(blas, objectTransform);
	}

	void SetObjectAutoTransform(uint32_t index, bool enable, const Transform& transform = Transform())
	{
		m_EnableAutoTransform[index] = enable ? 1 : 0;

		if (enable)
			m_AutoTransform[index] = transform;
	}

	void AddObjectToRotationList()
	{
		m_EnableAutoTransform.push_back(0);
		m_AutoTransform.emplace_back();
	}

	void HandleCameraControls(Hazel::Timestep ts)
	{
		m_Camera.UpdateCameraMatrix();

		float moveSpeed = m_CameraMoveSpeed * ts;
		float turnSpeed = m_CameraRotationSpeed * ts;

		if (Hazel::Input::IsKeyPressed(HZ_KEY_LEFT_SHIFT))
		{
			moveSpeed *= 4.0f;
			turnSpeed *= 3.0f;
		}

		if (Hazel::Input::IsKeyPressed(HZ_KEY_LEFT_ALT))
		{
			if (Hazel::Input::IsKeyPressed(HZ_KEY_W))
				m_Camera.rotation -= glm::vec3(1.0f, 0.0f, 0.0f) * turnSpeed;

			if (Hazel::Input::IsKeyPressed(HZ_KEY_S))
				m_Camera.rotation += glm::vec3(1.0f, 0.0f, 0.0f) * turnSpeed;

			if (Hazel::Input::IsKeyPressed(HZ_KEY_A))
				m_Camera.rotation -= glm::vec3(0.0f, 1.0f, 0.0f) * turnSpeed;

			if (Hazel::Input::IsKeyPressed(HZ_KEY_D))
				m_Camera.rotation += glm::vec3(0.0f, 1.0f, 0.0f) * turnSpeed;
		}
		else 
		{
			if (Hazel::Input::IsKeyPressed(HZ_KEY_W))
				m_Camera.position += m_Camera.Forward() * moveSpeed;

			if (Hazel::Input::IsKeyPressed(HZ_KEY_S))
				m_Camera.position += m_Camera.Back() * moveSpeed;

			if (Hazel::Input::IsKeyPressed(HZ_KEY_A))
				m_Camera.position += m_Camera.Left() * moveSpeed;

			if (Hazel::Input::IsKeyPressed(HZ_KEY_D))
				m_Camera.position += m_Camera.Right() * moveSpeed;
		}

		if (Hazel::Input::IsKeyPressed(HZ_KEY_Q))
		{
			m_Camera.position -= m_Camera.Up() * moveSpeed;
		}
		else if (Hazel::Input::IsKeyPressed(HZ_KEY_E))
		{
			m_Camera.position += m_Camera.Up() * moveSpeed;
		}

		if (Hazel::Input::IsKeyPressed(HZ_KEY_R))
		{
			m_Camera.position = glm::vec3(-0.195f, 1.5f, -0.195f);
			m_Camera.rotation = glm::vec3(0.0f, 111.0f, 0.0f);
		}
	}

	bool GetObjFilePath(std::string& filepath)
	{
		char filepath_charArray[MAX_PATH];
		OPENFILENAMEA toOpenFileName;
		ZeroMemory(&filepath_charArray, sizeof(filepath_charArray));
		ZeroMemory(&toOpenFileName, sizeof(toOpenFileName));
		toOpenFileName.lStructSize = sizeof(toOpenFileName);
		toOpenFileName.lpstrFile = filepath_charArray;
		toOpenFileName.nMaxFile = MAX_PATH;
		toOpenFileName.hwndOwner = NULL;
		toOpenFileName.lpstrTitle = "Select obj file.";
		toOpenFileName.lpstrFilter = ".obj\0*.obj\0";
		toOpenFileName.Flags = OFN_FILEMUSTEXIST | OFN_DONTADDTORECENT;

		if (GetOpenFileNameA(&toOpenFileName))
		{
			filepath = std::string(filepath_charArray);
			return true;
		}

		HZ_WARN("Failed to open OBJ file or User cancelled OBJ file selection.");
		return false;
	}
};

class Sandbox : public Hazel::Application
{
public:
	Sandbox() 
	{
		PushLayer(new RaytracerLayer());
	}

	~Sandbox() 
	{

	}
};

Hazel::Application* Hazel::CreateApplication() {
	return new Sandbox();
}