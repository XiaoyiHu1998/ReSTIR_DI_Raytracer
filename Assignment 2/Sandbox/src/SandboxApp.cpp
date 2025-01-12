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

class PathTracingLayer : public Hazel::Layer
{
public:
	PathTracingLayer() :
		Layer("PathTracing")
	{
		m_Renderer = Renderer();
		m_Camera = Camera(m_CurrentWidth, m_CurrentHeight, 40);

		m_FrameBuffer = std::make_shared<std::vector<uint8_t>>();
		m_CurrentWidth = m_NextWidth = 640;
		m_CurrentHeight = m_NextHeight = 480;

		RenderCommand::GenerateFrameBufferTexture(m_FrameBufferID, m_FrameBuffer, m_CurrentWidth, m_CurrentHeight);

		m_TLAS = TLAS();
		m_TLAS_EmmisiveOnly = TLAS();
		m_TLAS_NonEmmisiveOnly = TLAS();

		std::vector<Triangle> triangles;
		GeometryLoader::LoadGeometryFromFile(".\\assets\\models\\cube.obj", triangles);

		using BLAS_TYPE = BVH_BLAS;
		//ceiling
		std::shared_ptr<BLAS_TYPE> ceilingBLAS = std::make_shared<BLAS_TYPE>();
		Transform ceilingTransform = Transform(glm::vec3(0, 20, 0), glm::vec3(0), glm::vec3(200, 1, 50));
		Material ceilingMaterial = Material(Material::Type::Emissive, 1, 0, 1, glm::vec3(0.5f), glm::vec3(1.0));
		Mesh ceiling = Mesh(triangles, ceilingTransform, ceilingMaterial);
		ceilingBLAS->SetObject(ceiling.GetTriangles(), ceiling.GetTransform(), ceiling.GetMaterial());
		m_TLAS.AddBLAS(ceilingBLAS);
		m_TLAS_EmmisiveOnly.AddBLAS(ceilingBLAS);

		//floor
		std::shared_ptr<BLAS_TYPE> floorBLAS = std::make_shared<BLAS_TYPE>();
		Transform floorTransform = Transform(glm::vec3(0, -20, 0), glm::vec3(0), glm::vec3(200, 1, 50));
		Material floorMaterial = Material(Material::Type::Dielectric, 1, 0, 1, glm::vec3(0.5f), glm::vec3(0));
		Mesh floor = Mesh(triangles, floorTransform, floorMaterial);
		floorBLAS->SetObject(floor.GetTriangles(), floor.GetTransform(), floor.GetMaterial());
		m_TLAS.AddBLAS(floorBLAS);

		//sphere
		std::shared_ptr<BLAS_TYPE> sphereBLAS = std::make_shared<BLAS_TYPE>();
		//GeometryLoader::LoadGeometryFromFile(".\\assets\\models\\sphere.obj", triangles);
		//GeometryLoader::LoadGeometryFromFile(".\\assets\\models\\sphere_high_res.obj", triangles);
		GeometryLoader::LoadGeometryFromFile(".\\assets\\models\\sphere_ico_high_res.obj", triangles);
		Transform sphereTransform = Transform(glm::vec3(0, -15, 0), glm::vec3(0, 0, 0), glm::vec3(1.5, 1.5, 1.5));
		//Transform sphereTransform = Transform(glm::vec3(0, 0, 0.85), glm::vec3(0, 0, 0), glm::vec3(5));
		Material sphereMaterial = Material(Material::Type::Dielectric, 1, 0, 1, glm::vec3(0.25f), glm::vec3(0.5, 0.5, 1.0));
		Mesh sphere = Mesh(triangles, sphereTransform, sphereMaterial);
		sphereBLAS->SetObject(sphere.GetTriangles(), sphere.GetTransform(), sphere.GetMaterial());
		m_TLAS.AddBLAS(sphereBLAS);
	}

	~PathTracingLayer()
	{
		RenderCommand::DeleteFrameBufferTexture(m_FrameBufferID);
	}

	void OnUpdate(Hazel::Timestep timestep) override
	{
		m_CurrentWidth = m_NextWidth;
		m_CurrentHeight = m_NextHeight;
		m_Camera.SetResolution(m_CurrentWidth, m_CurrentHeight);

		RenderCommand::InitFrameBuffer(m_FrameBuffer, m_CurrentWidth, m_CurrentHeight);
		m_Renderer.RenderFrameBuffer(m_Camera, m_FrameBuffer, m_CurrentWidth, m_CurrentHeight, m_TLAS, m_TLAS_EmmisiveOnly);
		RenderCommand::SetFrameBufferTexture(m_FrameBufferID, m_FrameBuffer, m_CurrentWidth, m_CurrentHeight);
	}

	virtual void OnImGuiRender()
	{
		DrawImGuiDockSpace();

		// Viewport Window
		ImGui::Begin("Viewport");

		ImVec2 nextFrameResolution = ImGui::GetContentRegionAvail();
		ImVec2 viewportPosition = ImGui::GetWindowPos();

		m_NextWidth = std::max((uint32_t)2, static_cast<uint32_t>(nextFrameResolution.x));
		m_NextHeight = std::max((uint32_t)2, static_cast<uint32_t>(nextFrameResolution.y));

		ImGui::Image((void*)(intptr_t)m_FrameBufferID, ImVec2(m_CurrentWidth, m_CurrentHeight));
		ImGui::End();

		// Performance Metrics
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

		// Settings Window
		ImGui::Begin("Settings");
		ImGui::Text("Debug Settings");
		ImGui::Checkbox("Render Normals", &m_Renderer.GetSettings().RenderNormals);
		ImGui::End();

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
	uint32_t m_CurrentWidth, m_CurrentHeight;
	uint32_t m_NextWidth, m_NextHeight;

	//Renderer
	Renderer m_Renderer;

	// World state
	Camera m_Camera;
	TLAS m_TLAS;
	TLAS m_TLAS_EmmisiveOnly;
	TLAS m_TLAS_NonEmmisiveOnly;
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