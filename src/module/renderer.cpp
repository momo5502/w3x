#include <std_include.hpp>
#include "utils/hook.hpp"
#include "loader/loader.hpp"
#include "renderer.hpp"

void renderer::draw_text(const std::string& text, const position position, const color color)
{
	auto* instance = module_loader::get<renderer>();
	if (instance)
	{
		text_command command;
		command.text.append(text.begin(), text.end());
		command.position = position;
		command.color = color;

		std::lock_guard _(instance->mutex_);
		instance->command_queue_.push_back(command);
	}
}

void renderer::frame(const std::function<void()>& callback)
{
	auto* instance = module_loader::get<renderer>();
	if (instance)
	{
		instance->callbacks_.add(callback);
	}
}

void renderer::post_load()
{
	const auto render_stub = utils::hook::assemble([](utils::hook::assembler& a)
	{
		const auto skip_console = a.newLabel();

		a.mov(rax, rdi);
		a.mov(rdx, rax);
		a.test(rcx, rcx);
		a.jz(skip_console);

		a.pushad();
		a.call(0x140243180_g);
		a.popad();

		a.call(&execute_frame_static);

		a.bind(skip_console);
		a.jmp(0x14021D9D5_g);
	});

	utils::hook::jump(0x14021D9C8_g, render_stub);
}

void renderer::execute_frame(void* a1, void* a2)
{
	for (auto callback : this->callbacks_)
	{
		(*callback)();
	}

	std::lock_guard _(this->mutex_);
	for (const auto& command : this->command_queue_)
	{
		draw_text_internal(a1, a2, command);
	}

	this->command_queue_.clear();
}

void renderer::execute_frame_static(void* a1, void* a2)
{
	auto* instance = module_loader::get<renderer>();
	if (instance)
	{
		instance->execute_frame(a1, a2);
	}
}

void renderer::draw_text_internal(void* a1, void* a2, const text_command& command)
{
	text_object text{};
	text.text = command.text.data();
	text.length = uint32_t(command.text.size());

	utils::hook::invoke<void>(0x1402445D0_g, a1, a2, command.position.x, command.position.y, &text, command.color);
}

REGISTER_MODULE(renderer);