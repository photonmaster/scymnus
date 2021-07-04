//#include "server/app.hpp"

#include "core/named_tuple.hpp"
#include "core/named_tuples_utils.hpp"
#include "external/json.hpp"


using namespace scymnus;
using namespace nlohmann;


using ColorModel = model<
    field<"r", int, description("red")>,
    field<"g", int, description("green")>,
    field<"b", int, description("blue")>
    >;



using PointModel = model<
    field<"x", int>,
    field<"y", int>,
    field<"z", int>,
    field<"c", std::optional<ColorModel>>
    >;


using LineModel = model<
    field<"A", PointModel>, //server sets this
    field<"B", PointModel>
    >;





int main(){


auto a = PointModel{0,0,0, std::nullopt};
auto b = PointModel{0,2,2, std::nullopt};



auto line_1 = LineModel{a,b};
auto line_2 = LineModel{{0,0,0,std::nullopt},{2,0,2,std::nullopt}};
auto line_3 = LineModel{{0,0,0,std::nullopt},{2,2,0,std::nullopt}};

std::vector<LineModel> lines;
lines.push_back(line_1);
lines.push_back(line_2);
lines.push_back(line_3);


///two ways of  accessing model members
a.get<"x">() = -1;
///subscript operator will default construct the member if it's an optional that has not been set,
///or it will initialise it using the init meta-property if availiable
a[CT_("y")] = -1;




json v_lines = lines;

std::cout << "lines: " << v_lines.dump() << std::endl;


}
