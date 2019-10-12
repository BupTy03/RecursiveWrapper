#include "recursive_wrapper.hpp"

#include <iostream>
#include <vector>
#include <variant>
#include <memory>

struct Object2;

struct Object {
	Object() = default;

	std::variant<recursive_wrapper<Object2, 1>> objects;
};

struct Object2 {
	void print_hello() const {
		std::cout << "Hello, I'm Object2!" << std::endl;
	}
};

int main()
{
	{
		Object obj;
		const Object2& obj_ref = std::get<0>(obj.objects);
		obj_ref.print_hello();
	}

	system("pause");
	return 0;
}