/**********************************************************
* Holo_ToolSet
* http://github.com/HasseltVR/Holo_ToolSet
* http://www.uhasselt.be/edm
*
* Distributed under LGPL v2.1 Licence
* http ://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
**********************************************************/
using System;
using System.Collections.Generic;

public static class SingletonService<T> where T : class
{
    private static Dictionary<Type, object> _singletons = new Dictionary<Type, object>();

    //Adds a singleton to the manager
    public static void RegisterSingletonInstance(T instance)
    {
        if (!_singletons.ContainsKey(typeof(T)))
        {
            _singletons[typeof(T)] = instance;
        }
        else {
            throw new System.InvalidOperationException("A singleton of this type has already been registered.");
        }
    }

    //Removes a singleton type from the manager
    public static void UnregisterSingletonInstance(T instance)
    {
        if (!_singletons.Remove(typeof(T)))
        {
            throw new System.InvalidOperationException("You are trying to remove a singleton that does not exist!");
        }
    }

    //Attempts to get a singleton from the manager
    public static T GetSingleton()
    {
        Type requestedType = typeof(T);
        object o = null;
        if (_singletons.TryGetValue(requestedType, out o))
        {
            return (T)o;
        }
        return null;
    }
}