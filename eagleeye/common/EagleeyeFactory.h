#ifndef __EAGLEEYE_FACTORY_H_
#define __EAGLEEYE_FACTORY_H_
#include <string>
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/common/CJsonObject.hpp"
#include "eagleeye/framework/pipeline/AnyPipeline.h"

namespace eagleeye
{
/**
 * @brief build pipeline
 */ 
AnyPipeline* eagleeye_build_pipeline_from_json(const char* json_file, const char* resource_folder);
} // namespace eagleeye


#endif