#pragma once

namespace Hazel {

	class Texture
	{
	public:
		virtual ~Texture() = default;

		virtual uint32_t GetWidth() = 0;
		virtual uint32_t GetHeight() = 0;

		virtual void Bind(uint32_t slot = 0) = 0;
	};

	class Texture2D : public Texture
	{
	public:
		static Ref<Texture2D> Create();
		static Ref<Texture2D> Create(const std::string& path);
	};

}