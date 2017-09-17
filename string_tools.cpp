//字符串 替换
void string_replace(const std::string& oldValue,const std::string& newValue,std::string& value)
{
    for(std::string::size_type pos = value.find(oldValue);
            pos != std::string::npos;
            pos = value.find(oldValue,pos))
    {
        value.replace(pos,oldValue.size(),newValue);
        pos += newValue.size();
    }
}

//字符串大小写
std::string string_toUpper(const std::string& value)
{
    std::string copy = value;
    std::transform(copy.begin(),copy.end(),copy.begin(),toupper);
    return copy;
}

std::string string_toLower(const std::string& value)
{
    std::string copy = value;
    std::transform(copy.begin(),copy.end(),copy.begin(),tolower);
    return copy; 
}

//字符串 strip 
std::string string_strip(const std::string& value)
{
    if(!value.size())
        return value;

    size_t startPos = value.find_first_not_of(" \t\r\n");
    size_t endPos = value.find_last_not_of(" \t\r\n");

    if(startPos == std::string::npos)
        return value;

    return std::string(value,startPos,endPos-startPos+1);
}
