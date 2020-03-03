// Copyright (C) 2019, Robin Klimonow. All rights reserved.

#pragma once

#include "style.h"

#include "base/non_copyable.h"
#include "xml_handler/xml_handler.h"


namespace mmo
{
	/// Loads style xml files to construct styles using the StyleManager class.
	class StyleXmlLoader final 
		: public XmlHandler
		, public NonCopyable
	{
	public:
		// ~ Begin XmlHandler
		virtual void ElementStart(const std::string& element, const XmlAttributes& attributes) override;
		virtual void ElementEnd(const std::string& element) override;
		virtual void Text(const std::string& text) override;
		// ~ End XmlHandler

	private:
		void ElementStyleStart(const XmlAttributes& attributes);
		void ElementStyleEnd();
		void ElementImagerySectionStart(const XmlAttributes& attributes);
		void ElementImagerySectionEnd();
		void ElementImageryStart(const XmlAttributes& attributes);
		void ElementImageryEnd();
		void ElementLayerStart(const XmlAttributes& attributes);
		void ElementLayerEnd();
		void ElementSectionStart(const XmlAttributes& attributes);
		void ElementSectionEnd();

	private:
		StylePtr m_style;
		std::shared_ptr<StateImagerySection> m_section;
		std::shared_ptr<StateImagery> m_stateImagery;
	};
}
