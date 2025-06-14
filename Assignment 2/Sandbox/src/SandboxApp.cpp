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
#include "Mesh.h"
#include "Utils.h"

using BLAS_TYPE = BVH_BLAS;

class PathTracingLayer : public Hazel::Layer
{
public:
	PathTracingLayer() :
		Layer("PathTracing")
	{
		m_FrameBuffer = std::make_shared<std::vector<uint8_t>>();
		m_CurrentWidth = m_NextWidth = 640;
		m_CurrentHeight = m_NextHeight = 480;

		m_Renderer;
		m_Camera = Camera(m_CurrentWidth, m_CurrentHeight, 60);
		m_Camera.GetTransformRef().translation = glm::vec3(-1.0f, 1.5f, -0.5f);
		m_Camera.GetTransformRef().rotation = glm::vec3(0.0f, -111.0f, 0.0f);

		RenderCommand::GeneratePixelBufferObject(m_PixelBufferObjectID, m_FrameBuffer, m_CurrentWidth, m_CurrentHeight);
		RenderCommand::GenerateFrameBufferTexture(m_FrameBufferID, m_FrameBuffer, m_CurrentWidth, m_CurrentHeight);
		
		m_TLAS = TLAS();
		m_TLAS_EmmisiveOnly = TLAS();
		m_TLAS_NonEmmisiveOnly = TLAS();

		uint32_t lightCount = 10;
		m_SphereLights = std::vector<Sphere>();
		m_SphereLights.reserve(lightCount);
		uint32_t sphereLocationSeed = 0;
		uint32_t sphereColorSeed = 0;
		for (int i = 0; i < lightCount; i++)
		{
			float x = Utils::RandomFloat(sphereLocationSeed) * 10;
			float y = Utils::RandomFloat(sphereLocationSeed) * 0;
			float z = Utils::RandomFloat(sphereLocationSeed) * 10 - 2;
			glm::vec3 position = glm::vec3(x, y, z);

			float radius = 1.0f;

			//float uniformColor = 1.0f;
			float r = std::max(0.2f, Utils::RandomFloat(sphereColorSeed));
			float g = std::max(0.2f, Utils::RandomFloat(sphereColorSeed));
			float b = std::max(0.2f, Utils::RandomFloat(sphereColorSeed));
			glm::vec3 emissiveColor = glm::vec3(r, g, b);
			float emissiveStrength = 3.0f;
			Material material = Material(Material::Type::Emissive, 0, emissiveColor, emissiveColor, emissiveStrength);

			m_SphereLights.emplace_back(position, radius, material);
			std::cout << "Sphere " << i << " Pos:" << glm::to_string(position) << ", Radius: " << radius << ",Color: " << glm::to_string(emissiveColor) << ", intensity: " << emissiveStrength << std::endl;
		}


		std::vector<Triangle> triangles;
		//GeometryLoader::LoadGeometryFromFile(".\\assets\\models\\cube.obj", triangles);
		//
		////ceiling
		//std::shared_ptr<BLAS_TYPE> ceilingBLAS = std::make_shared<BLAS_TYPE>();
		//ceilingBLAS->SetName("Ceiling");
		//Transform ceilingTransform = Transform(glm::vec3(0, 2, 0), glm::vec3(0), glm::vec3(1, 1, 1));
		//Material ceilingMaterial = Material(Material::Type::Emissive, 1, glm::vec3(0.5f), glm::vec3(1.0), 1.0f);
		//Mesh ceiling = Mesh(triangles, ceilingTransform, ceilingMaterial);
		//ceilingBLAS->SetObject(ceiling.GetTriangles(), ceiling.GetTransform(), ceiling.GetMaterial());
		//m_TLAS.AddBLAS(ceilingBLAS);
		//m_TLAS_EmmisiveOnly.AddBLAS(ceilingBLAS);

		//floor
		GeometryLoader::LoadGeometryFromFile(".\\assets\\models\\sponza_small.obj", triangles);
		//GeometryLoader::LoadGeometryFromFile(".\\assets\\models\\cube.obj", triangles);
		std::shared_ptr<BLAS_TYPE> floorBLAS = std::make_shared<BLAS_TYPE>();
		floorBLAS->SetName("Sponza");
		Transform floorTransform = Transform(glm::vec3(0, 0, 0), glm::vec3(0), glm::vec3(1));
		Material floorMaterial = Material(Material::Type::Non_Emissive, 1, glm::vec3(0.5f), glm::vec3(0) , 0.0f);
		Mesh floor = Mesh(triangles, floorTransform, floorMaterial);
		floorBLAS->SetObject(floor.GetTriangles(), floor.GetTransform(), floor.GetMaterial());
		m_TLAS.AddBLAS(floorBLAS);

		//sphere
		//GeometryLoader::LoadGeometryFromFile(".\\assets\\models\\sphere.obj", triangles);
		GeometryLoader::LoadGeometryFromFile(".\\assets\\models\\sphere_high_res.obj", triangles);
		//GeometryLoader::LoadGeometryFromFile(".\\assets\\models\\sphere_ico_high_res.obj", triangles);
		std::shared_ptr<BLAS_TYPE> sphereBLAS = std::make_shared<BLAS_TYPE>();
		sphereBLAS->SetName("Sphere");
		Transform sphereTransform = Transform(glm::vec3(3.6f, -0.3f, -0.95f), glm::vec3(0, 0, 0), glm::vec3(1, 1, 1));
		//Transform sphereTransform = Transform(glm::vec3(0, 0, 0.85), glm::vec3(0, 0, 0), glm::vec3(5));
		Material sphereMaterial = Material(Material::Type::Non_Emissive, 1, glm::vec3(0.25f), glm::vec3(0.5, 0.5, 1.0), 0.0f);
		Mesh sphere = Mesh(triangles, sphereTransform, sphereMaterial);
		sphereBLAS->SetObject(sphere.GetTriangles(), sphere.GetTransform(), sphere.GetMaterial());
		m_TLAS.AddBLAS(sphereBLAS);

		m_PrevFrameResolution = glm::i32vec2(std::numeric_limits<int>().max(), std::numeric_limits<int>().max());
		m_Renderer.Init(Renderer::Scene(m_Camera, m_TLAS, m_SphereLights));

		FrameBufferRef frameBuffer = m_Renderer.GetFrameBuffer();
		glm::i32vec2 m_CurrentFrameResolution = m_Renderer.GetRenderResolution();
		RenderCommand::InitFrame(m_FrameBufferID, m_PixelBufferObjectID, frameBuffer, m_CurrentFrameResolution.x, m_CurrentFrameResolution.y);
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

		if (m_CurrentFrameResolution.x != m_PrevFrameResolution.x || m_CurrentFrameResolution.y != m_PrevFrameResolution.y)
		{
			RenderCommand::InitFrame(m_FrameBufferID, m_PixelBufferObjectID, frameBuffer, m_CurrentFrameResolution.x, m_CurrentFrameResolution.y);
			m_PrevFrameResolution = m_CurrentFrameResolution;
		}

		RenderCommand::UploadFrameData(m_FrameBufferID, m_PixelBufferObjectID, frameBuffer, m_CurrentFrameResolution.x, m_CurrentFrameResolution.y);

		// Setup next frame to be rendered
		m_CurrentWidth = m_NextWidth;
		m_CurrentHeight = m_NextHeight;
		m_Camera.SetResolution(m_CurrentWidth, m_CurrentHeight);
		m_Camera.UpdateFrustrum();

		m_Renderer.SubmitRenderSettings(m_RendererSettingsUI);
		m_Renderer.SubmitScene(Renderer::Scene(m_Camera, m_TLAS, m_SphereLights));
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
			ImGui::Text("Render Settings");
			const char* RenderModes[] = { "Normals", "TraversalSteps", "Direct Illumination", "ReSTIR" };
			int selectedMode = static_cast<int>(m_RendererSettingsUI.Mode);
			ImGui::Combo("Render Mode", &selectedMode, RenderModes, IM_ARRAYSIZE(RenderModes));
			m_RendererSettingsUI.Mode = static_cast<Renderer::Settings::RenderMode>(selectedMode);
			ImGui::Checkbox("Random Seed", &m_RendererSettingsUI.RandomSeed);
			ImGui::DragFloat("Eta size", &m_RendererSettingsUI.Eta, 0.001f, 0.001f, 0.1f);
			ImGui::Separator();

			if (m_RendererSettingsUI.Mode == Renderer::Settings::RenderMode::DI)
			{
				ImGui::Text("DI Rendering");
				ImGui::Checkbox("Sample all lights", &m_RendererSettingsUI.SampleAllLightsDI);
				ImGui::Checkbox("Light Occlusion", &m_RendererSettingsUI.LightOcclusionCheckDI);
				if (ImGui::InputInt("Light Candidates", &m_RendererSettingsUI.CandidateCountDI))
				{
					m_RendererSettingsUI.CandidateCountDI = m_RendererSettingsUI.CandidateCountDI < 1 ? 1 : m_RendererSettingsUI.CandidateCountDI;
				}
				ImGui::Separator();
			}
			else if (m_RendererSettingsUI.Mode == Renderer::Settings::RenderMode::ReSTIR)
			{
				// Streaming RIS
				ImGui::Text("Streaming RIS");
				if (ImGui::InputInt("Candidate Count RIS", &m_RendererSettingsUI.CandidateCountReSTIR))
				{
					m_RendererSettingsUI.CandidateCountReSTIR = m_RendererSettingsUI.CandidateCountReSTIR < 1 ? 1 : m_RendererSettingsUI.CandidateCountReSTIR;
				}
				ImGui::Checkbox("Visibility Pass", &m_RendererSettingsUI.EnableVisibilityPass);
				ImGui::Separator();

				// Spatial Reuse
				ImGui::Text("Spatial Reuse");
				ImGui::Checkbox("Enable", &m_RendererSettingsUI.EnableSpatialReuse);
				if (ImGui::InputInt("Iterations", &m_RendererSettingsUI.SpatialReuseIterationCount))
				{
					m_RendererSettingsUI.SpatialReuseNeighbours = m_RendererSettingsUI.SpatialReuseNeighbours < 1 ? 1 : m_RendererSettingsUI.SpatialReuseNeighbours;
				}
				if (ImGui::InputInt("Neighbours", &m_RendererSettingsUI.SpatialReuseNeighbours))
				{
					m_RendererSettingsUI.SpatialReuseNeighbours = m_RendererSettingsUI.SpatialReuseNeighbours < 1 ? 1 : m_RendererSettingsUI.SpatialReuseNeighbours;
				}
				if (ImGui::InputInt("Radius", &m_RendererSettingsUI.SpatialReuseRadius))
				{
					m_RendererSettingsUI.SpatialReuseRadius = m_RendererSettingsUI.SpatialReuseRadius < 1 ? 1 : m_RendererSettingsUI.SpatialReuseRadius;
				}
				ImGui::DragFloat("Max distance", &m_RendererSettingsUI.SpatialReuseMaxDistance, 0.001f, 0.0f, 1.0f);
				ImGui::DragFloat("Min Normal Similarity", &m_RendererSettingsUI.SpatialReuseMinNormalSimilarity, 0.001f, 0.0f, 1.0f);
				ImGui::Separator();

				// Temporal Reuse
				ImGui::Checkbox("Temporal Reuse", &m_RendererSettingsUI.EnableTemporalReuse);

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
			for (uint32_t i = 0; i < m_TLAS.GetObjectCount(); i++)
			{
				DrawImGUiTreeNodeEX(i + 1, m_TLAS.GetBLAS(i)->GetName().c_str());
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
				cameraUpdated |= ImGui::DragFloat3("Position", glm::value_ptr(m_Camera.GetTransformRef().translation), 0.05f);
				cameraUpdated |= ImGui::DragFloat3("Rotation", glm::value_ptr(m_Camera.GetTransformRef().rotation), 0.05f);
				ImGui::Separator();

				ImGui::Text("Camera Settings");
				ImGui::DragFloat("Vertical FoV", &m_Camera.GetFOVRef(), 0.1f, 0.0f, 360.0f);

				ImGui::PopID();

				if (cameraUpdated)
					m_Camera.UpdateCameraMatrix();
			}
			else if (1 <= m_SelectedNode && m_SelectedNode < m_TLAS.GetObjectCount() + 1)
			{
				uint32_t blasIndex = m_SelectedNode - 1;
				std::shared_ptr<BLAS> blas = m_TLAS.GetBLAS(blasIndex);
				bool transformUpdated = false;

				const char* materialTypes[] = { "Non_Emissive", "Emissive" };
				int selectedMaterialType = 0;
				switch (blas->GetMaterial().MaterialType)
				{
				case Material::Type::Non_Emissive:
					selectedMaterialType = 0;
					break;
				case Material::Type::Emissive:
					selectedMaterialType = 1;
					break;
				}

				ImGui::PushID(blas->GetName().c_str());

				ImGui::InputText("Name", &blas->GetNameRef()[0], blas->GetName().length());
				ImGui::Separator();

				ImGui::Text("Transform");
				transformUpdated |= ImGui::DragFloat3("position", glm::value_ptr(blas->GetTransformRef().translation), 0.05f);
				transformUpdated |= ImGui::DragFloat3("rotation", glm::value_ptr(blas->GetTransformRef().rotation), 0.05f);
				// Not accessible until ray transform issues are fixed
				//transformUpdated |= ImGui::DragFloat3("scale", glm::value_ptr(blas->GetTransformRef().scale), 0.05f); 
				ImGui::Separator();

				ImGui::Text("Material");
				ImGui::Combo("Material Type", &selectedMaterialType, materialTypes, IM_ARRAYSIZE(materialTypes));
				ImGui::Spacing();
				ImGui::Spacing();
				ImGui::Spacing();
				ImGui::ColorEdit3("Albedo", glm::value_ptr(blas->GetMaterialRef().Albedo));
				ImGui::ColorEdit3("EmissiveColor", glm::value_ptr(blas->GetMaterialRef().EmissiveColor));
				ImGui::DragFloat("EmmisiveIntensity", &blas->GetMaterialRef().EmissiveIntensity);
				ImGui::DragFloat("Roughness", &blas->GetMaterialRef().Roughness, 0.001f, 0.0f, 1.0f);

				ImGui::PopID();

				switch (selectedMaterialType)
				{
				case 0:
					blas->GetMaterialRef().MaterialType = Material::Type::Non_Emissive;
					break;
				case 1:
					blas->GetMaterialRef().MaterialType = Material::Type::Emissive;
					break;
				}

				if (transformUpdated)
					blas->SetTransform(blas->GetTransform());
			}
			ImGui::End();
		}


		RenderCommand::Clear();
	}

	void OnEvent(Hazel::Event& event) override
	{

	}

	void HandleInput(Hazel::Timestep ts) 
	{
		
	}
private:
	// Output Configuration
	FrameBufferRef m_FrameBuffer;
	uint32_t m_FrameBufferID;
	uint32_t m_PixelBufferObjectID;
	uint32_t m_CurrentWidth, m_CurrentHeight;
	uint32_t m_NextWidth, m_NextHeight;
	glm::i32vec2 m_PrevFrameResolution;

	//Renderer
	Renderer m_Renderer;
	Renderer::Settings m_RendererSettingsUI;

	// World state
	Camera m_Camera;
	TLAS m_TLAS;
	TLAS m_TLAS_EmmisiveOnly;
	TLAS m_TLAS_NonEmmisiveOnly;
	std::vector<Sphere> m_SphereLights;

	// UI
	int m_SelectedNode;
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

		if (ImGui::BeginMenuBar()) {
			if (ImGui::BeginMenu("File")) {
				if (ImGui::MenuItem("import obj")) {
					//importFile = true;
				}
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}
		ImGui::End();
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