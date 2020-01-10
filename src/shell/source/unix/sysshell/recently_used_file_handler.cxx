/*************************************************************************
 *
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * Copyright 2000, 2010 Oracle and/or its affiliates.
 *
 * OpenOffice.org - a multi-platform office productivity suite
 *
 * This file is part of OpenOffice.org.
 *
 * OpenOffice.org is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3
 * only, as published by the Free Software Foundation.
 *
 * OpenOffice.org is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License version 3 for more details
 * (a copy is included in the LICENSE file that accompanied this code).
 *
 * You should have received a copy of the GNU Lesser General Public License
 * version 3 along with OpenOffice.org.  If not, see
 * <http://www.openoffice.org/license.html>
 * for a copy of the LGPLv3 License.
 *
 ************************************************************************/

// MARKER(update_precomp.py): autogen include statement, do not remove
#include "precompiled_shell.hxx"
 
#include "osl/process.h"
#include "rtl/ustring.hxx"
#include "rtl/string.hxx"
#include "rtl/strbuf.hxx"

#include "osl/thread.h"
#include "recently_used_file.hxx"

#include "internal/xml_parser.hxx"
#include "internal/i_xml_parser_event_handler.hxx"

#include <map>
#include <vector>
#include <algorithm>
#include <functional>
#include <string.h>

namespace /* private */ {
    
    const rtl::OUString ENVV_UPDATE_RECENTLY_USED = 
        rtl::OUString::createFromAscii("ENABLE_UPDATE_RECENTLY_USED");
    
    //########################################   
    inline rtl::OString get_file_extension(const rtl::OString& file_url)
    {
        sal_Int32 index = file_url.lastIndexOf('.');
        OSL_ENSURE((index != -1) && ((index + 1) < file_url.getLength()), "Invalid file url");
        return file_url.copy(index + 1);        
    }
        
    //########################################   
    typedef std::vector<string_t> string_container_t;
	
    #define TAG_RECENT_FILES "RecentFiles"
    #define TAG_RECENT_ITEM  "RecentItem"
    #define TAG_URI          "URI"
    #define TAG_MIME_TYPE    "Mime-Type"
    #define TAG_TIMESTAMP    "Timestamp"
    #define TAG_PRIVATE      "Private"
    #define TAG_GROUPS       "Groups"
    #define TAG_GROUP        "Group"
    
    //------------------------------------------------    
    // compare two string_t's case insensitive, may also be done
    // by specifying special traits for the string type but in this
    // case it's easier to do it this way
    struct str_icase_cmp : 
        public std::binary_function<string_t, string_t, bool>
    {
        bool operator() (const string_t& s1, const string_t& s2) const
        { return (0 == strcasecmp(s1.c_str(), s2.c_str())); }
    };
    
    //------------------------------------------------    
    struct recently_used_item
    {
        recently_used_item() :
            is_private_(false)
        {}
	
        recently_used_item(
            const string_t& uri,
            const string_t& mime_type,            
            const string_container_t& groups,
            bool is_private = false) :
            uri_(uri),
            mime_type_(mime_type),
            is_private_(is_private),
            groups_(groups)
        {
            timestamp_ = time(NULL);
        }
    
        void set_uri(const string_t& character)
        { uri_ = character; }
    
        void set_mime_type(const string_t& character)
        { mime_type_ = character; }
    
        void set_timestamp(const string_t& character)
        { 
            time_t t;
            if (sscanf(character.c_str(), "%ld", &t) != 1)
                timestamp_ = -1; 
            else
                timestamp_ = t;
        }
    
        void set_is_private(const string_t& /*character*/)
        { is_private_ = true; }
    
        void set_groups(const string_t& character)
        { groups_.push_back(character); }

        void set_nothing(const string_t& /*character*/)
        {}
		
        bool has_groups() const
        {
            return !groups_.empty();
        }
        
        bool has_group(const string_t& name) const 
        {
            string_container_t::const_iterator iter_end = groups_.end();
            return (has_groups() && 
                    iter_end != std::find_if(
                        groups_.begin(), iter_end, 
                        std::bind2nd(str_icase_cmp(), name)));
        }
        
        void write_xml(const recently_used_file& file) const
        {
            write_xml_start_tag(TAG_RECENT_ITEM, file, true);
            write_xml_tag(TAG_URI, uri_, file);
            write_xml_tag(TAG_MIME_TYPE, mime_type_, file);
                                
            rtl::OString ts = rtl::OString::valueOf((sal_sSize)timestamp_);            
            write_xml_tag(TAG_TIMESTAMP, ts.getStr(), file);
                        
            if (is_private_)            
                write_xml_tag(TAG_PRIVATE, file);                               
            
            if (has_groups())
            {
                write_xml_start_tag(TAG_GROUPS, file, true);
                                
                string_container_t::const_iterator iter = groups_.begin();
                string_container_t::const_iterator iter_end = groups_.end();
                
                for ( ; iter != iter_end; ++iter)                
                    write_xml_tag(TAG_GROUP, (*iter), file);                    
                
                write_xml_end_tag(TAG_GROUPS, file);                
            }            
            write_xml_end_tag(TAG_RECENT_ITEM, file);            
        }
        
        static rtl::OString escape_content(const string_t &text)
        {
            rtl::OStringBuffer aBuf;
            for (sal_uInt32 i = 0; i < text.length(); i++)
            {
#               define MAP(a,b) case a: aBuf.append(b); break
                switch (text[i])
                {
                    MAP ('&',  "&amp;");
                    MAP ('<',  "&lt;");
                    MAP ('>',  "&gt;");
                    MAP ('\'', "&apos;");
                    MAP ('"',  "&quot;");
                default:
                    aBuf.append(text[i]);
                    break;
                }
#               undef MAP
            }
            return aBuf.makeStringAndClear();
        }
        
        void write_xml_tag(const string_t& name, const string_t& value, const recently_used_file& file) const
        {            
            write_xml_start_tag(name, file);
            rtl::OString escaped = escape_content (value);
            file.write(escaped.getStr(), escaped.getLength());
            write_xml_end_tag(name, file);
        }
        
        void write_xml_tag(const string_t& name, const recently_used_file& file) const 
        {            
            file.write("<", 1);
            file.write(name.c_str(), name.length());
            file.write("/>\n", 3);            
        }
        
        void write_xml_start_tag(const string_t& name, const recently_used_file& file, bool linefeed = false) const 
        {
            file.write("<", 1);
            file.write(name.c_str(), name.length());
            if (linefeed) 
                file.write(">\n", 2);
            else
                file.write(">", 1);
        }
        
        void write_xml_end_tag(const string_t& name, const recently_used_file& file) const 
        {
            file.write("</", 2);
            file.write(name.c_str(), name.length());
            file.write(">\n", 2);         
        }
        
        string_t uri_;
        string_t mime_type_;
        time_t timestamp_;
        bool is_private_;
        string_container_t groups_;
    };
	
    typedef std::vector<recently_used_item*> recently_used_item_list_t;    
    typedef void (recently_used_item::* SET_COMMAND)(const string_t&);

    //########################################   
    // thrown if we encounter xml tags that we do not know
    class unknown_xml_format_exception {};

    //########################################   
    class recently_used_file_filter : public i_xml_parser_event_handler
    {
    public:	
        recently_used_file_filter(recently_used_item_list_t& item_list) : 
            item_(NULL),
            item_list_(item_list)
        {
            named_command_map_[TAG_RECENT_FILES] = &recently_used_item::set_nothing;
            named_command_map_[TAG_RECENT_ITEM]  = &recently_used_item::set_nothing;
            named_command_map_[TAG_URI]          = &recently_used_item::set_uri;
            named_command_map_[TAG_MIME_TYPE]    = &recently_used_item::set_mime_type;
            named_command_map_[TAG_TIMESTAMP]    = &recently_used_item::set_timestamp;
            named_command_map_[TAG_PRIVATE]      = &recently_used_item::set_is_private;
            named_command_map_[TAG_GROUPS]       = &recently_used_item::set_nothing;
            named_command_map_[TAG_GROUP]        = &recently_used_item::set_groups;
        }
            
        virtual void start_element(
            const string_t& /*raw_name*/, 
            const string_t& local_name, 
            const xml_tag_attribute_container_t& /*attributes*/)
        {
            if ((local_name == TAG_RECENT_ITEM) && (NULL == item_))                            
                item_ = new recently_used_item;            
        }

        virtual void end_element(const string_t& /*raw_name*/, const string_t& local_name)
        {
            // check for end tags w/o start tag
            if( local_name != TAG_RECENT_FILES && NULL == item_ )
                return; // will result in an XML parser error anyway
            
            if (named_command_map_.find(local_name) != named_command_map_.end())
                (item_->*named_command_map_[local_name])(current_element_);
            else
            {
                delete item_;
                throw unknown_xml_format_exception();
            }
        
            if (local_name == TAG_RECENT_ITEM)
            {
                item_list_.push_back(item_);
                item_ = NULL;
            }
            current_element_.clear();
        }

        virtual void characters(const string_t& character)
        {
            if (character != "\n")
                current_element_ += character;        
        }
    
        virtual void start_document() {}        
        virtual void end_document()   {}
 
        virtual void ignore_whitespace(const string_t& /*whitespaces*/)
        {}  
        
        virtual void processing_instruction(
            const string_t& /*target*/, const string_t& /*data*/)
        {}
        
        virtual void comment(const string_t& /*comment*/)
        {}        
    private:
        recently_used_item* item_;
        std::map<string_t, SET_COMMAND> named_command_map_;
        string_t current_element_;
        recently_used_item_list_t& item_list_;
    private:
        recently_used_file_filter(const recently_used_file_filter&);
        recently_used_file_filter& operator=(const recently_used_file_filter&);
    };

    //------------------------------------------------        
    void read_recently_used_items(
        recently_used_file& file, 
        recently_used_item_list_t& item_list)
    {
        xml_parser xparser;	
        recently_used_file_filter ruff(item_list);
        
        xparser.set_document_handler(&ruff);
		                
        char buff[16384];			
		while (!file.eof())
		{
        	if (size_t length = file.read(buff, sizeof(buff)))            
                xparser.parse(buff, length, file.eof());                
		}
    }
    
    //------------------------------------------------    
    // The file ~/.recently_used shall not contain more than 500
    // entries (see www.freedesktop.org)
    const int MAX_RECENTLY_USED_ITEMS = 500;
        
    class recent_item_writer
    {
    public:
        recent_item_writer(
            recently_used_file& file, 
            int max_items_to_write = MAX_RECENTLY_USED_ITEMS) :
            file_(file),
            max_items_to_write_(max_items_to_write),
            items_written_(0)
        {}
        
        void operator() (const recently_used_item* item)
        {
            if (items_written_++ < max_items_to_write_)
                item->write_xml(file_);
        }
    private:
        recently_used_file& file_;
        int max_items_to_write_;
        int items_written_;
    };
    
    //------------------------------------------------    
    const char* XML_HEADER = "<?xml version=\"1.0\"?>\n<RecentFiles>\n";
    const char* XML_FOOTER = "</RecentFiles>";
    
    //------------------------------------------------    
    // assumes that the list is ordered decreasing
    void write_recently_used_items(
        recently_used_file& file,
        recently_used_item_list_t& item_list)
    {        
        if (!item_list.empty())
        {
            file.truncate();
            file.reset();
        
            file.write(XML_HEADER, strlen(XML_HEADER));
                
            std::for_each(
                item_list.begin(), 
                item_list.end(), 
                recent_item_writer(file));
        
            file.write(XML_FOOTER, strlen(XML_FOOTER));
        }
    }
    
    //------------------------------------------------    
    struct delete_recently_used_item
	{	
		void operator() (const recently_used_item* item) const
		{ delete item; }
	};
    
    //------------------------------------------------    
    void recently_used_item_list_clear(recently_used_item_list_t& item_list)
    {
        std::for_each(
            item_list.begin(), 
			item_list.end(), 
			delete_recently_used_item());            
		item_list.clear();
    }
        
    //------------------------------------------------    
    class find_item_predicate
    {
    public:
        find_item_predicate(const string_t& uri) :
            uri_(uri) 
        {}
            
        bool operator() (const recently_used_item* item)
        { return (item->uri_ == uri_); }        
    private:
        string_t uri_;
    };
    
    //------------------------------------------------    
    struct greater_recently_used_item
    {
        bool operator ()(const recently_used_item* lhs, const recently_used_item* rhs) const
        { return (lhs->timestamp_ > rhs->timestamp_); }
    };
    
    //------------------------------------------------
    const char* GROUP_OOO         = "openoffice.org";
    const char* GROUP_STAR_OFFICE = "staroffice";
    const char* GROUP_STAR_SUITE  = "starsuite";
    
    //------------------------------------------------    
    void recently_used_item_list_add(
        recently_used_item_list_t& item_list, const rtl::OUString& file_url, const rtl::OUString& mime_type)
    {
        rtl::OString f = rtl::OUStringToOString(file_url, RTL_TEXTENCODING_UTF8);
        
        recently_used_item_list_t::iterator iter = 
            std::find_if(
                item_list.begin(), 
                item_list.end(), 
                find_item_predicate(f.getStr()));    
        
        if (iter != item_list.end())
        {           
            (*iter)->timestamp_ = time(NULL);
            
            if (!(*iter)->has_group(GROUP_OOO)) 
                (*iter)->groups_.push_back(GROUP_OOO);
            if (!(*iter)->has_group(GROUP_STAR_OFFICE))
                (*iter)->groups_.push_back(GROUP_STAR_OFFICE);
            if (!(*iter)->has_group(GROUP_STAR_SUITE))
                (*iter)->groups_.push_back(GROUP_STAR_SUITE);
        }
        else
        {
            string_container_t groups;
            groups.push_back(GROUP_OOO);
            groups.push_back(GROUP_STAR_OFFICE);
            groups.push_back(GROUP_STAR_SUITE);
            
            string_t uri(f.getStr());
            string_t mimetype(rtl::OUStringToOString(mime_type, osl_getThreadTextEncoding()).getStr());                                    
            
            if (mimetype.length() == 0)
                mimetype = "application/octet-stream";
            
            item_list.push_back(new recently_used_item(uri, mimetype, groups));
        }
        
        // sort decreasing after the timestamp 
        // so that the newest items appear first
        std::sort(
            item_list.begin(), 
            item_list.end(), 
            greater_recently_used_item());
    }
    
    //##############################
    bool update_recently_used_enabled()
    {
        rtl::OUString tmp;        
        osl_getEnvironment(ENVV_UPDATE_RECENTLY_USED.pData, &tmp.pData);
		return (tmp.getLength() > 0);
    }

    //------------------------------------------------
    struct cleanup_guard
    {    
        cleanup_guard(recently_used_item_list_t& item_list) :
            item_list_(item_list)
        {}
        ~cleanup_guard()
        { recently_used_item_list_clear(item_list_); }
    
        recently_used_item_list_t& item_list_;
    };
        
} // namespace private

//###########################################
/*
   example (see http::www.freedesktop.org):
    <?xml version="1.0"?>
                <RecentFiles>
                     <RecentItem>
                        <URI>file:///home/federico/gedit.txt</URI>
                        <Mime-Type>text/plain</Mime-Type>
                        <Timestamp>1046485966</Timestamp>
                        <Groups>
                            <Group>gedit</Group>
                        </Groups>
                    </RecentItem>
                    <RecentItem>
                        <URI>file:///home/federico/gedit-2.2.0.tar.bz2</URI>
                        <Mime-Type>application/x-bzip</Mime-Type>
                        <Timestamp>1046209851</Timestamp>
                        <Private/>
                        <Groups>
                        </Groups>
                    </RecentItem>
                </RecentFiles>
*/

extern "C" void add_to_recently_used_file_list(const rtl::OUString& file_url, const rtl::OUString& mime_type)
{
    try
    {
        recently_used_file ruf;			
        recently_used_item_list_t item_list;
        cleanup_guard guard(item_list);	
	
        read_recently_used_items(ruf, item_list);	
        recently_used_item_list_add(item_list, file_url, mime_type);
        write_recently_used_items(ruf, item_list);                                
    }
    catch(const char* ex)
    {
        OSL_ENSURE(false, ex);
    }
    catch(const xml_parser_exception&)
	{
        OSL_ENSURE(false, "XML parser error");
    }		            
    catch(const unknown_xml_format_exception&)
    {
        OSL_ENSURE(false, "XML format unknown");
    }        
}
    
