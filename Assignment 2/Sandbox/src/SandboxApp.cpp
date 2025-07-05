#include <Hazel.h>

#include "imgui/imgui.h"
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

using BLAS_TYPE = BVH_BLAS;

class PathTracingLayer : public Hazel::Layer
{
public:
	PathTracingLayer() :
		Layer("PathTracing")
	{
		// Create Scene
		m_TLAS = TLAS();
		
		// Lights
		m_LightLocationSeed = 0;
		m_LightColorSeed = 0;
		m_LightStrength = 0.65f;
		m_LightCount = 100;
		GenerateLights();

		// Geometry
		LoadObject(".\\assets\\models\\sponza_small.obj", "Sponza", Transform(glm::vec3(0, 0, 0), glm::vec3(0), glm::vec3(1)));
		LoadObject(".\\assets\\models\\sphere_high_res.obj", "Sphere", Transform(glm::vec3(5.7f, 0.3f, -0.95f), glm::vec3(0, 0, 0), glm::vec3(1, 1, 1)));

		// Setup Rendering
		HZ_INFO("Initiating Renderer");
		m_RendererSettingsUI;
		m_CurrentWidth = m_NextWidth = m_RendererSettingsUI.FrameWidth;
		m_CurrentHeight = m_NextHeight = m_RendererSettingsUI.FrameHeight;

		m_Camera = Camera(m_CurrentWidth, m_CurrentHeight, 60);
		m_Camera.position = glm::vec3(-1.0f, 1.5f, -0.5f);
		m_Camera.rotation = glm::vec3(0.0f, 111.0f, 0.0f);
		m_MoveCamera = false;

		m_Renderer;
		m_Renderer.Init(m_RendererSettingsUI, Renderer::Scene(m_Camera, m_TLAS, m_pointLights));

		FrameBufferRef frameBuffer = m_Renderer.GetFrameBuffer();
		RenderCommand::GeneratePixelBufferObject(m_PixelBufferObjectID, frameBuffer, m_CurrentWidth, m_CurrentHeight);
		RenderCommand::GenerateFrameBufferTexture(m_FrameBufferID, frameBuffer, m_CurrentWidth, m_CurrentHeight);
		RenderCommand::InitFrame(m_FrameBufferID, m_PixelBufferObjectID, frameBuffer, m_CurrentWidth, m_CurrentHeight);
		RenderCommand::UploadFrameData(m_FrameBufferID, m_PixelBufferObjectID, frameBuffer, m_CurrentWidth, m_CurrentHeight);
	}

	~PathTracingLayer()
	{
		RenderCommand::DeleteFrameBufferTexture(m_FrameBufferID);
	}

	void OnUpdate(Hazel::Timestep timestep) override
	{
		// Display rendererd frame
		FrameBufferRef frameBuffer = m_Renderer.GetFrameBuffer();
		glm::i32vec2 m_CurrentFrameResolution = m_Renderer.GetRenderResolution();

		if (m_CurrentFrameResolution != m_PrevFrameResolution)
		{
			RenderCommand::InitFrame(m_FrameBufferID, m_PixelBufferObjectID, frameBuffer, m_CurrentFrameResolution.x, m_CurrentFrameResolution.y);
			m_PrevFrameResolution = m_CurrentFrameResolution;
		}

		RenderCommand::UploadFrameData(m_FrameBufferID, m_PixelBufferObjectID, frameBuffer, m_CurrentFrameResolution.x, m_CurrentFrameResolution.y);

		// Setup next frame to be rendered
		m_CurrentWidth = m_NextWidth;
		m_CurrentHeight = m_NextHeight;

		m_RendererSettingsUI.FrameWidth = m_CurrentWidth;
		m_RendererSettingsUI.FrameHeight = m_CurrentHeight;

		if (m_MoveCamera)
			m_Camera.position += glm::vec3(0.000075f * timestep, 0, 0);

		m_Renderer.SubmitRenderSettings(m_RendererSettingsUI);
		m_Renderer.SubmitScene(Renderer::Scene(m_Camera, m_TLAS, m_pointLights));
	}

	virtual void OnImGuiRender()
	{
		DrawImGuiDockSpace();

		// Viewport Window
		{
			ImGui::Begin("Viewport");
			ImVec2 nextFrameResolution = ImGui::GetContentRegionAvail();
			ImVec2 viewportPosition = ImGui::GetWindowPos();
			m_NextWidth = std::max((uint32_t)2, static_cast<uint32_t>(nextFrameResolution.x));
			m_NextHeight = std::max((uint32_t)2, static_cast<uint32_t>(nextFrameResolution.y));
			ImGui::Image((void*)(intptr_t)m_FrameBufferID, ImVec2(m_CurrentWidth, m_CurrentHeight));
			ImGui::End();

			// Performance Metrics Subwindow
			ImGui::SetNextWindowBgAlpha(0.35f);
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
		}

		// Settings Window
		{
			ImGui::Begin("Settings");
			ImGui::PushItemWidth(-ImGui::GetWindowWidth() * 0.35f);
			ImGui::Text("Render Settings");
			const char* RenderModes[] = { "Normals", "TraversalSteps", "Direct Illumination", "ReSTIR" };
			int selectedMode = static_cast<int>(m_RendererSettingsUI.Mode);
			ImGui::Combo("Render Mode", &selectedMode, RenderModes, IM_ARRAYSIZE(RenderModes));
			m_RendererSettingsUI.Mode = static_cast<RendererSettings::RenderMode>(selectedMode);

			ImGui::PushItemWidth(-ImGui::GetWindowWidth() * 0.65f);
			if (ImGui::InputInt("Thread Count", &m_RendererSettingsUI.ThreadCount, 1, 1))
			{
				m_RendererSettingsUI.ThreadCount = std::max(0, m_RendererSettingsUI.ThreadCount);
				m_RendererSettingsUI.ThreadCount = std::min(static_cast<int>(std::thread::hardware_concurrency()), m_RendererSettingsUI.ThreadCount);
			}
			ImGui::DragFloat("Eta size", &m_RendererSettingsUI.Eta, 0.001f, 0.001f, 0.1f);
			ImGui::Checkbox("Random Seed", &m_RendererSettingsUI.RandomSeed);
			ImGui::Separator();

			if (m_RendererSettingsUI.Mode == RendererSettings::RenderMode::DI)
			{
				ImGui::Text("DI Rendering");
				ImGui::Checkbox("Sample all lights", &m_RendererSettingsUI.SampleAllLightsDI);
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
				ImGui::Checkbox("Visibility Pass", &m_RendererSettingsUI.EnableVisibilityPass);
				ImGui::Separator();

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
				ImGui::DragFloat("Max distance", &m_RendererSettingsUI.SpatialMaxDistance, 0.001f, 0.0f, 1.0f);
				ImGui::DragFloat("Max distance depth scaling", &m_RendererSettingsUI.SpatialMaxDistanceDepthScaling, 0.001f, 0.0f, 5.0f);
				ImGui::DragFloat("Min Normal Similarity", &m_RendererSettingsUI.SpatialMinNormalSimilarity, 0.001f, 0.0f, 1.0f);
				ImGui::Separator();
				ImGui::PopID();

				// Temporal Reuse
				ImGui::PushID("Temporal Reuse Options");
				ImGui::Checkbox("Temporal Reuse", &m_RendererSettingsUI.EnableTemporalReuse);
				if (ImGui::InputInt("Sample Count Ratio", &m_RendererSettingsUI.TemporalSampleCountRatio))
				{
					m_RendererSettingsUI.TemporalSampleCountRatio = m_RendererSettingsUI.TemporalSampleCountRatio < 1 ? 1 : m_RendererSettingsUI.TemporalSampleCountRatio;
				}
				ImGui::DragFloat("Max distance", &m_RendererSettingsUI.TemporalMaxDistance, 0.001f, 0.0f, 1.0f);
				ImGui::DragFloat("Max distance depth scaling", &m_RendererSettingsUI.TemporalMaxDistanceDepthScaling, 0.001f, 0.0f, 5.0f);
				ImGui::DragFloat("Min Normal Similarity", &m_RendererSettingsUI.TemporalMinNormalSimilarity, 0.001f, 0.0f, 1.0f);
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
				DrawImGUiTreeNodeEX(i + 2, m_TLAS.GetBLAS(i)->GetNameRef().c_str());
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

				ImGui::Text("Camera Settings");
				ImGui::PushItemWidth(-ImGui::GetWindowWidth() * 0.5f);
				ImGui::DragFloat("Vertical FoV", &m_Camera.verticalFOV, 0.1f, 1.0f, 160.0f);
				ImGui::Checkbox("Auto Move Camera", &m_MoveCamera);

				ImGui::PopID();
			}
			else if (m_SelectedNode == 1)
			{
				ImGui::PushID("Properties_Lights");

				ImGui::PushItemWidth(-ImGui::GetWindowWidth() * 0.5f);
				ImGui::DragInt("Light Count", &m_LightCount, 1, 0, 10000);
				ImGui::DragInt("Light Color Seed", &m_LightColor, 1, 0, 1000);
				ImGui::DragInt("Light Location Seed", &m_LightLocation, 1, 0, 1000);

				ImGui::DragFloat("Light Intensity", &m_LightStrength, 0.010f, 0.0f, 30.0f);

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
				std::shared_ptr<BLAS> blas = m_TLAS.GetBLAS(blasIndex);
				bool transformUpdated = false;

				ImGui::PushID(blas->GetNameRef().c_str());
				ImGui::InputText("Name", &blas->GetNameRef()[0], blas->GetNameRef().size() + 1);
				ImGui::Separator();

				ImGui::Text("Transform");
				transformUpdated |= ImGui::DragFloat3("position", glm::value_ptr(blas->GetTransformRef().translation), 0.05f);
				transformUpdated |= ImGui::DragFloat3("rotation", glm::value_ptr(blas->GetTransformRef().rotation), 0.05f);
				ImGui::Separator();

				ImGui::PopID();

				if (transformUpdated)
					blas->UpdateTransform();
			}
			ImGui::End();
		}

		RenderCommand::ClearFrame();
	}

	void OnEvent(Hazel::Event& event) override
	{

	}

	void HandleInput(Hazel::Timestep ts) 
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

	// World state
	Camera m_Camera;
	TLAS m_TLAS;
	std::vector<PointLight> m_pointLights;
	float m_LightStrength;
	int m_LightCount;
	int m_LightColor;
	int m_LightLocation;
	uint32_t m_LightColorSeed;
	uint32_t m_LightLocationSeed;

	// UI
	int m_SelectedNode;
	bool m_MoveCamera;
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
			float x = Utils::RandomFloat(m_LightLocationSeed) * 50 - 25;
			float y = Utils::RandomFloat(m_LightLocationSeed) * 7 + 1.0f;
			float z = Utils::RandomFloat(m_LightLocationSeed) * 9 - 4;
			glm::vec3 position = glm::vec3(x, y, z);

			// Color
			float r = std::max(0.2f, Utils::RandomFloat(m_LightColorSeed));
			float g = std::max(0.2f, Utils::RandomFloat(m_LightColorSeed));
			float b = std::max(0.2f, Utils::RandomFloat(m_LightColorSeed));
			glm::vec3 emissiveColor = glm::vec3(r, g, b);

			m_pointLights.emplace_back(position, emissiveColor, m_LightStrength);
		}
	}

	void LoadObject(const std::string& fileName, const std::string& objectName, const Transform& transform) 
	{
		HZ_INFO("Loading Object {} from {}", objectName, fileName);
		std::vector<Triangle> triangleBuffer;
		GeometryLoader::LoadObj(fileName, triangleBuffer);

		std::shared_ptr<BLAS_TYPE> BLAS = std::make_shared<BLAS_TYPE>();
		BLAS->SetObject(triangleBuffer, transform);
		BLAS->SetName(objectName);

		m_TLAS.AddBLAS(BLAS);
	}
};

class Sandbox : public Hazel::Application
{
public:
	Sandbox() 
	{
		PushLayer(new PathTracingLayer());
	}

	~Sandbox() 
	{

	}
};

Hazel::Application* Hazel::CreateApplication() {
	return new Sandbox();
}