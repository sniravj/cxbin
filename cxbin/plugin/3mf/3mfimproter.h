#ifndef _3MF_Importer_H
#define _3MF_Importer_H
#include "trimesh2/TriMesh.h"
#include "trimesh2/XForm.h"
#include <vector>
#include <memory>
#include <string>
#include <map>
#include "expat.h"
#include "miniz_extension.h"

namespace cxbin
{
#define Vec3f trimesh::vec3
#define Vec3i trimesh::ivec3

    class _BBS_3MF_Importer
    {
    public:
        bool load3MF(const std::string& filename);

        bool load3MF(const std::string& filename
            ,std::vector<trimesh::TriMesh*>& meshs
            ,std::vector<std::vector<std::string>>& colors
            ,std::vector<std::vector<std::string>>& seams
            ,std::vector<std::vector<std::string>>& supports
        );

    private:
        typedef std::pair<std::string, int> Id; // BBS: encrypt

        struct Component
        {
            Id object_id;
            trimesh::xform transform;

            explicit Component(Id object_id)
                : object_id(object_id)
                , transform(trimesh::xform::identity())
            {
            }

            Component(Id object_id, const trimesh::xform& transform)
                : object_id(object_id)
                , transform(transform)
            {
            }
        };

        typedef std::vector<Component> ComponentsList;

        struct Geometry
        {
            std::vector<trimesh::vec> vertices;
            std::vector<trimesh::TriMesh::Face> triangles;
            std::vector<std::string> custom_supports;
            std::vector<std::string> custom_seam;
            std::vector<std::string> mmu_segmentation;
            // BBS
            std::vector<std::string> face_properties;

            bool empty() { return vertices.empty() || triangles.empty(); }

            // backup & restore
            void swap(Geometry& o) {
                std::swap(vertices, o.vertices);
                std::swap(triangles, o.triangles);
                std::swap(custom_supports, o.custom_supports);
                std::swap(custom_seam, o.custom_seam);
            }

            void reset() {
                vertices.clear();
                triangles.clear();
                custom_supports.clear();
                custom_seam.clear();
                mmu_segmentation.clear();
            }
        };

        struct CurrentObject
        {
            // ID of the object inside the 3MF file, 1 based.
            int id;
            // Index of the ModelObject in its respective Model, zero based.
            int model_object_idx;
            Geometry geometry;
            char* object;
            ComponentsList components;

            //BBS: sub object id
            //int subobject_id;
            std::string name;
            std::string uuid;
            int         pid{ -1 };
            //bool is_model_object;

            CurrentObject() { reset(); }

            void reset() {
                id = -1;
                model_object_idx = -1;
                geometry.reset();
                object = nullptr;
                components.clear();
                //BBS: sub object id
                uuid.clear();
                name.clear();
            }
        };

        //
        XML_Parser m_xml_parser;
        std::string m_start_part_path;
        std::string m_thumbnail_path;
        std::vector<std::string> m_sub_model_paths;
        std::string m_curr_characters;
        std::string m_curr_metadata_name;
        std::string m_backup_path;//±¸·Ý
        std::string m_origin_file;
        std::string m_name;
        float m_unit_factor;
        CurrentObject* m_curr_object{ nullptr };

        std::map<std::string, trimesh::xform> ModelPos;
        typedef std::map<Id, CurrentObject> IdToCurrentObjectMap;
        IdToCurrentObjectMap m_current_objects;
        int m_current_color_group{ -1 };
        std::map<int, std::string> m_group_id_to_color;
        std::string m_sub_model_path;
        //


    // Encode an 8-bit string from a local code page to UTF-8.
    // Multibyte to utf8
        std::string decode_path(const char* src);

        std::string encode_path(const char* src);

        const char* bbs_get_attribute_value_charptr(const char** attributes, unsigned int attributes_size, const char* attribute_key);

        bool _handle_start_relationship(const char** attributes, unsigned int num_attributes);

        void _stop_xml_parser(const std::string& msg = std::string());

        static void _handle_start_relationships_element(void* userData, const char* name, const char** attributes);
        static void _handle_end_relationships_element(void* userData, const char* name);

        void _handle_start_relationships_element(const char* name, const char** attributes);
        void _handle_end_relationships_element(const char* name);

        bool m_load_restore;

        struct ObjectImporter
        {
            IdToCurrentObjectMap object_list;
            CurrentObject* current_object{ nullptr };
            std::string object_path;
            std::string zip_path;
            _BBS_3MF_Importer* top_importer{ nullptr };
            XML_Parser object_xml_parser;
            bool obj_parse_error{ false };
            std::string obj_parse_error_message;

            //local parsed datas
            std::string obj_curr_metadata_name;
            std::string obj_curr_characters;
            float object_unit_factor;
            int object_current_color_group{ -1 };
            std::map<int, std::string> object_group_id_to_color;
            bool is_bbl_3mf{ false };

            ObjectImporter(_BBS_3MF_Importer* importer, std::string file_path, std::string obj_path)
            {
                top_importer = importer;
                object_path = obj_path;
                zip_path = file_path;
            }

            ~ObjectImporter()
            {
                _destroy_object_xml_parser();
            }

            void _destroy_object_xml_parser()
            {
                if (object_xml_parser != nullptr) {
                    XML_ParserFree(object_xml_parser);
                    object_xml_parser = nullptr;
                }
            }

            void _stop_object_xml_parser(const std::string& msg = std::string())
            {
                assert(!obj_parse_error);
                assert(obj_parse_error_message.empty());
                assert(object_xml_parser != nullptr);
                obj_parse_error = true;
                obj_parse_error_message = msg;
                XML_StopParser(object_xml_parser, false);
            }

            bool        object_parse_error()         const { return obj_parse_error; }
            const char* object_parse_error_message() const {
                return obj_parse_error ?
                    // The error was signalled by the user code, not the expat parser.
                    (obj_parse_error_message.empty() ? "Invalid 3MF format" : obj_parse_error_message.c_str()) :
                    // The error was signalled by the expat parser.
                    XML_ErrorString(XML_GetErrorCode(object_xml_parser));
            }

            bool _extract_object_from_archive(mz_zip_archive& archive, const mz_zip_archive_file_stat& stat);

            bool extract_object_model()
            {
                mz_zip_archive archive;
                mz_zip_archive_file_stat stat;
                mz_zip_zero_struct(&archive);

                if (!open_zip_reader(&archive, zip_path)) {
                    //top_importer->add_error("Unable to open the zipfile " + zip_path);
                    return false;
                }

                if (!top_importer->_extract_from_archive(archive, object_path, [this](mz_zip_archive& archive, const mz_zip_archive_file_stat& stat) {
                    return _extract_object_from_archive(archive, stat);
                    }, top_importer->m_load_restore)) {
                    std::string error_msg = std::string("Archive does not contain a valid model for ") + object_path;
                    //top_importer->add_error(error_msg);

                    close_zip_reader(&archive);
                    return false;
                }

                close_zip_reader(&archive);

                if (obj_parse_error) {
                    //already add_error inside
                    //top_importer->add_error(object_parse_error_message());
                    //BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ":" << __LINE__ << boost::format(", Found error while extrace object %1%\n") % object_path;
                    return false;
                }
                return true;
            }

            bool _handle_object_start_model(const char** attributes, unsigned int num_attributes);
            bool _handle_object_end_model();

            bool _handle_object_start_resources(const char** attributes, unsigned int num_attributes);
            bool _handle_object_end_resources();

            bool _handle_object_start_object(const char** attributes, unsigned int num_attributes);
            bool _handle_object_end_object();

            bool _handle_object_start_color_group(const char** attributes, unsigned int num_attributes);
            bool _handle_object_end_color_group();

            bool _handle_object_start_color(const char** attributes, unsigned int num_attributes);
            bool _handle_object_end_color();

            bool _handle_object_start_mesh(const char** attributes, unsigned int num_attributes);
            bool _handle_object_end_mesh();

            bool _handle_object_start_vertices(const char** attributes, unsigned int num_attributes);
            bool _handle_object_end_vertices();

            bool _handle_object_start_vertex(const char** attributes, unsigned int num_attributes);
            bool _handle_object_end_vertex();

            bool _handle_object_start_triangles(const char** attributes, unsigned int num_attributes);
            bool _handle_object_end_triangles();

            bool _handle_object_start_triangle(const char** attributes, unsigned int num_attributes);
            bool _handle_object_end_triangle();

            bool _handle_object_start_components(const char** attributes, unsigned int num_attributes);
            bool _handle_object_end_components();

            bool _handle_object_start_component(const char** attributes, unsigned int num_attributes);
            bool _handle_object_end_component();

            bool _handle_object_start_metadata(const char** attributes, unsigned int num_attributes);
            bool _handle_object_end_metadata();

            void _handle_object_start_model_xml_element(const char* name, const char** attributes);
            void _handle_object_end_model_xml_element(const char* name);
            void _handle_object_xml_characters(const XML_Char* s, int len);

            // callbacks to parse the .model file of an object
            static void XMLCALL _handle_object_start_model_xml_element(void* userData, const char* name, const char** attributes);
            static void XMLCALL _handle_object_end_model_xml_element(void* userData, const char* name);
            static void XMLCALL _handle_object_xml_characters(void* userData, const XML_Char* s, int len);
        };

        std::vector<ObjectImporter*> m_object_importers;

        bool _extract_from_archive(mz_zip_archive& archive, std::string const& path, std::function<bool(mz_zip_archive& archive, const mz_zip_archive_file_stat& stat)> extract, bool restore = false);

        void _destroy_xml_parser();

        bool _extract_xml_from_archive(mz_zip_archive& archive, const mz_zip_archive_file_stat& stat, XML_StartElementHandler start_handler, XML_EndElementHandler end_handler);
        bool _extract_xml_from_archive(mz_zip_archive& archive, const std::string& path, XML_StartElementHandler start_handler, XML_EndElementHandler end_handler);

        bool _handle_start_model(const char** attributes, unsigned int num_attributes);

        bool _handle_start_resources(const char** attributes, unsigned int num_attributes);


        bool _handle_start_object(const char** attributes, unsigned int num_attributes);

        bool _handle_start_color_group(const char** attributes, unsigned int num_attributes);

        bool _handle_start_color(const char** attributes, unsigned int num_attributes);

        bool _handle_start_mesh(const char** attributes, unsigned int num_attributes);

        bool _handle_start_vertices(const char** attributes, unsigned int num_attributes);

        bool _handle_start_vertex(const char** attributes, unsigned int num_attributes);

        bool _handle_start_triangles(const char** attributes, unsigned int num_attributes);

        bool _handle_start_triangle(const char** attributes, unsigned int num_attributes);

        bool _handle_start_metadata(const char** attributes, unsigned int num_attributes);

        bool _handle_start_components(const char** attributes, unsigned int num_attributes);

        bool _handle_start_component(const char** attributes, unsigned int num_attributes);

        //bool _create_object_instance(std::string const& path, int object_id, const trimesh::xform& transform, const bool printable, unsigned int recur_counter);

        bool _handle_start_item(const char** attributes, unsigned int num_attributes);

        void _handle_start_model_xml_element(const char* name, const char** attributes);

        void _handle_end_model_xml_element(const char* name);

        void _handle_xml_characters(const XML_Char* s, int len);

        bool _handle_end_model();

        bool _handle_end_resources();

        bool _handle_end_object();

        bool _handle_end_color_group();

        bool _handle_end_color();

        bool _handle_end_mesh();

        bool _handle_end_vertices();

        bool _handle_end_vertex();

        bool _handle_end_triangles();

        bool _handle_end_triangle();

        //void _handle_end_model_xml_element(void* userData, const char* name);

        //void _handle_xml_characters(void* userData, const XML_Char* s, int len);

        bool _extract_model_from_archive(mz_zip_archive& archive, const mz_zip_archive_file_stat& stat);

        void _handle_object_start_model_xml_element(const char* name, const char** attributes) {};
        void _handle_object_end_model_xml_element(const char* name) {};
        void _handle_object_xml_characters(const XML_Char* s, int len) {};

        // callbacks to parse the .model file of an object
        static void  _handle_object_start_model_xml_element(void* userData, const char* name, const char** attributes);
        static void  _handle_object_end_model_xml_element(void* userData, const char* name);
        static void  _handle_object_xml_characters(void* userData, const XML_Char* s, int len);

        // callbacks to parse the .model file
        static void _handle_start_model_xml_element(void* userData, const char* name, const char** attributes);
        static void _handle_end_model_xml_element(void* userData, const char* name);
        static void _handle_xml_characters(void* userData, const XML_Char* s, int len);
    };
}

#endif // _LOAD3MF_H