//
// Created by mwo on 6/11/15.
//

#include "CmdLineOptions.h"


namespace xmreg
{
    /**
     * Take the acc and *avv[] from the main() and check and parse
     * all the options given
     */
    CmdLineOptions::CmdLineOptions(int acc, const char *avv[]) {

        positional_options_description p;

        options_description desc(
                "blkaccesstime, check blocks access time");

        desc.add_options()
                ("help,h", value<bool>()->default_value(false)->implicit_value(true),
                 "produce help message")
                ("address,a", value<string>(),
                 "monero address string")
                ("viewkey,v", value<string>(),
                 "private view key string")
                ("start-height,t", value<size_t>(),
                 "private view key string")
                ("out-csv-file,c", value<string>(),
                 "private view key string")
                ("bc-path,b", value<string>(),
                 "path to lmdb blockchain");


        store(command_line_parser(acc, avv)
                          .options(desc)
                          .run(), vm);

        notify(vm);

        if (vm.count("help"))
        {
            if (vm["help"].as<bool>())
                cout << desc << "\n";
        }
    }

    /**
     * Return the value of the argument passed to the program
     * in wrapped around boost::optional
     */
    template<typename T>
    boost::optional<T>
    CmdLineOptions::get_option(const string & opt_name) const
    {

        if (!vm.count(opt_name))
        {
            return boost::none;
        }

        return vm[opt_name].as<T>();
    }


    // explicit instantiations of get_option template function
    template  boost::optional<string>
    CmdLineOptions::get_option<string>(const string & opt_name) const;

    template  boost::optional<bool>
            CmdLineOptions::get_option<bool>(const string & opt_name) const;

    template  boost::optional<size_t>
            CmdLineOptions::get_option<size_t>(const string & opt_name) const;

}
