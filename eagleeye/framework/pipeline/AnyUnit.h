#ifndef _ANYUNIT_H_
#define _ANYUNIT_H_

#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/EagleeyeTimeStamp.h"
#include "eagleeye/framework/pipeline/AnyMonitor.h"
#include <vector>
#include <memory>

namespace eagleeye
{
class EAGLEEYE_API AnyUnit
{
public:
	/**
	 *	@brief define some basic type
	 *	@note you must do these
	 */
	typedef AnyUnit							Self;
	typedef AnyUnit							Superclass;


	AnyUnit(const char* unit_name="AnyUnit");
	virtual ~AnyUnit();

	/**
	 *	@brief Get class identity
	 */
	EAGLEEYE_CLASSIDENTITY(AnyUnit);

	/**
	 *	@brief Set/Get unit name
	 */
	EAGLEEYE_GETUNITNAME;
	EAGLEEYE_SETUNITNAME(AnyUnit);

	/**
	 *	@brief If some parameters or preliminary data of this unit are changed,\n
	 *	we should call this function.
	 */
	virtual void modified() const;

	/**
	 *	@brief Get the modified time.
	 */
	virtual unsigned long getMTime() const;

	/**
	 * @brief frozen mtime
	 * 
	 */
	virtual void frozenMTime() const;

	/**
	 *	@brief Deliver the update flow
	 *	@note It would be called implicitly
	 */
	virtual void updateUnitInfo(){};

	/**
	 *	@brief According to update time, process unit info
	 *	dynamically, such as doing some concrete task, generating
	 *	some data, or other things.
	 */
	virtual void processUnitInfo(){};

	/**
	 *	@brief print unit info
	 */
	virtual void printUnit(){};

	/**
	 *	@brief make some self check
	 *	@note If some preliminary data or parameters are invalid,
	 *	it would return false.
	 */
	virtual bool selfcheck(){return true;};

	/**
	 *	@brief get monitor object
	 */
	AnyMonitor* getMonitor(int index);

	/**
	 *	@brief get the size of monitor pool
	 */
	int getMonitorPoolSize();

	/**
	 *	@brief set the framework name
	 */
	void setPipelineName(const char* framework_name);
	
	/**
	 *	@brief set/get parameter block
	 */
	virtual void setUnitPara(std::shared_ptr<char> param_block);
	virtual void getUnitPara(std::shared_ptr<char>& param_block);

protected:
	std::vector<AnyMonitor*> m_unit_monitor_pool;	/**< monitor pool*/
	std::string m_unit_name;						/**< unit name*/

private:
	AnyUnit(const AnyUnit&);
	void operator=(const AnyUnit&);

	mutable EagleeyeTimeStamp m_mtime;
	std::string m_pipeline_name;					/**< pipeline name*/
};
}

#endif
