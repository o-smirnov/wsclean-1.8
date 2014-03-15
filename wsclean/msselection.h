#ifndef MS_SELECTION
#define MS_SELECTION

#include <cstring>

class MSSelection
{
public:
	MSSelection() :
		_fieldId(0),
		_startChannel(0), _endChannel(0),
		_startTimestep(0), _endTimestep(0),
		_autoCorrelations(false)
	{
	}
	
	bool HasChannelRange() const { return _endChannel != 0; }
	bool HasInterval() const { return _endTimestep != 0; }
	
	size_t ChannelRangeStart() const { return _startChannel; }
	size_t ChannelRangeEnd() const { return _endChannel; }
	
	size_t IntervalStart() const { return _startTimestep; }
	size_t IntervalEnd() const { return _endTimestep; }
	
	size_t FieldId() const { return _fieldId; }
	
	bool IsSelected(size_t fieldId, size_t timestep, size_t antenna1, size_t antenna2) const
	{
		if(fieldId != _fieldId)
			return false;
		else if(HasInterval() && (timestep < _startTimestep || timestep >= _endTimestep))
			return false;
		else if(!_autoCorrelations && (antenna1 == antenna2))
			return false;
		else
			return true;
	}
	bool IsFieldSelected(size_t fieldId) const
	{
		return fieldId == _fieldId;
	}
	void SetFieldId(size_t fieldId)
	{ 
		_fieldId = fieldId; 
	}
	void SetChannelRange(size_t startChannel, size_t endChannel)
	{
		_startChannel = startChannel;
		_endChannel = endChannel;
	}
	void SetNoChannelRange()
	{
		_startChannel = 0;
		_endChannel = 0;
	}
	void SetInterval(size_t startTimestep, size_t endTimestep)
	{
		_startTimestep = startTimestep;
		_endTimestep = endTimestep;
	}
	static MSSelection Everything() { return MSSelection(); }
private:
	size_t _fieldId;
	size_t _startChannel, _endChannel;
	size_t _startTimestep, _endTimestep;
	bool _autoCorrelations;
};

#endif