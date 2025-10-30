#pragma once
#include <string>
#include <optional>

namespace pmon::util::win
{
	class MsgBox
	{
	public:
		MsgBox();
		MsgBox(std::string message);
		~MsgBox();

		MsgBox(const MsgBox&) = delete;
		MsgBox & operator=(const MsgBox&) = delete;
		MsgBox(MsgBox&&) = delete;
		MsgBox & operator=(MsgBox&&) = delete;

		MsgBox& WithMessage(std::string message);
		MsgBox& WithTitle(std::string title);
		MsgBox& AsError();
		MsgBox& AsModal(const void* hWnd = nullptr);
	private:
		enum class Type_
		{
			Notification,
			Error,
		};
		Type_ type_ = Type_::Notification;
		bool modal_ = false;
		const void* hWndModal_ = nullptr;
		std::string message_;
		std::optional<std::string> title_;
	};
}