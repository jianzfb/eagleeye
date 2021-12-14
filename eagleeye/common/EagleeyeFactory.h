#ifndef __EAGLEEYE_FACTORY_H_
#define __EAGLEEYE_FACTORY_H_
#include <string>
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/common/CJsonObject.hpp"
#include "eagleeye/framework/pipeline/AnyPipeline.h"
#include<functional>

namespace eagleeye
{
/**
 * @brief build pipeline (v1)
 */ 
AnyPipeline* eagleeye_build_pipeline_from_json(const char* json_file, const char* resource_folder);

/**
 * @brief build pipeline (v2)
 * 
 */
typedef std::function<bool(std::string node_name, std::string cls_name, neb::CJsonObject node_config, AnyNode*&, std::string resource_folder)> NodeBFuncType;
bool eagleeye_build_pipeline_from_json(AnyPipeline*pipeline, 
                                        const char* json_file,
                                        NodeBFuncType node_builder);
} // namespace eagleeye


#endif