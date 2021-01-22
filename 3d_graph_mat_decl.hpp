#pragma once

#include <iostream>	// for std::cout set as default for std::ostream
#include <cstdint>
#include <atomic>	// for std::atomic_bool
#include <functional>	// for std::function
#include <optional>
#include <variant>

#include "matrix_base.hpp"
#include "3d_graph_box.hpp"
#include "direction.hpp"

/*
@notes about directions ->

The screen is considered to be parallel to the xy plane, hence DOWN, UP, LEFT, RIGHT are lines parallel the x or y axis only

And, -z axis is considered as BACK_FACING (PATAL)
     +z axis is considered as FRONT_FACING (AAKASH)
*/

enum class MatrixLayer {
	TOP,
	DEEPEST,
	ORIGIN
};

template<typename node_dtype, typename dimen_t = int32_t>
class Graph_Matrix_3D : Matrix_Base{

	static_assert(std::is_signed_v<dimen_t>, "Dimension type must be a signed integral (for safety reasons, so that bugs don't appear due to unsigned subtractions)");
	static_assert(std::is_default_constructible_v<node_dtype>, "The data type of values in the matrix must be default constructible");
	static_assert( ! std::is_pointer_v<node_dtype>, "Currently it doesn't support using pointers to be the data held by each box, though it maybe implemented, but not now");

	typedef Graph_Box_3D<node_dtype> graph_box_type;
	typedef util::_coord<dimen_t> coord_type;
	typedef	std::variant<
		std::function<node_dtype && ()>,
		std::function<node_dtype && (int, int, int)>,
		std::function<void (node_dtype&, int, int, int)>,
		std::function<void (Graph_Box_3D<node_dtype>&)>
	> Init_Func;

protected:
	graph_box_type origin;

	graph_box_type* top_left_front;
	graph_box_type* top_left_back;
	graph_box_type* bottom_left_front;
	graph_box_type* bottom_left_back;
	graph_box_type* top_right_front;
	graph_box_type* top_right_back;
	graph_box_type* bottom_right_front;
	graph_box_type* bottom_right_back;

	struct {
		float expansion_speed{ Matrix_Base::init_expansion_speed }; //initially it will auto_expand statics::init_expansion_speed unit at time, each side
		float increase_units{0.0};	// units to increase in each direction, in call to expand_once()
		//float last_increase_units;	// @note @me - expansion speed itself behaves as this

		std::atomic_bool expansion_flag{ false };

		std::optional< Init_Func > initializer_function;

		void set_initializer(const Init_Func& initialiser) {
			initializer_function = initialiser;
		}
		void reset_initializer() {
			this->initializer_function.reset();
		}

		int time_since_speed_updated{ 0 }; //after 10 time units, the __expansion_state.expansion_speed will be decremented/reset, so as to not unecessary keep increasing storage
	}__expansion_state;

	// AUTO EXPANSION LOGIC START
		virtual void auto_expansion() override;	 //keeps expanding TILL expansion_flag is TRUE
		virtual void expand_once();
		virtual void expand_n_unit(const uint8_t);
	// AUTO EXPANSION LOGIC END

	struct {
		graph_box_type* top_left_front;
		graph_box_type* top_left_back;
		graph_box_type* bottom_left_front;
		graph_box_type* bottom_left_back;
		graph_box_type* top_right_front;
		graph_box_type* top_right_back;
		graph_box_type* bottom_right_front;
		graph_box_type* bottom_right_back;

		coord_type total_abs;
	} __capacity;   //capacity data

	MemoryAlloc< Graph_Box_3D<node_dtype> >	allocator;

	dimen_t min_x, min_y, min_z;	// ALWAYS NON-POSITIVES (since origin layer won't be removed so at max 0)
	dimen_t max_x, max_y, max_z;	// ALWAYS NON-NEGATIVES (  "      "      "    "    "   "      "  " min 0)

	coord_type total_abs;	// total span of a single dimension

	void add_x_layer(int num = 1);
	void inject_x_layer(int num = 1);
	void pop_xplus_layer();
	void pop_xminus_layer();

	void add_y_layer(int num = 1);
	void inject_y_layer(int num = 1);
	void pop_yplus_layer();
	void pop_yminus_layer();

	void add_z_layer(int num = 1);
	void inject_z_layer(int num = 1);
	void pop_zplus_layer();
	void pop_zminus_layer();

	void disp_xy_layer(MatrixLayer ltype = MatrixLayer::TOP);
	void disp_xy_layer(int lnum, std::ostream& os = std::cout);

	template < typename _Func >
	void for_each(graph_box_type* source, Direction, _Func);	// func will receive only one param, ie. the node_dtype data

	template<typename _Func>
	void for_each(graph_box_type* begin, graph_box_type* end, Direction dir, _Func func);

	//template < typename _Cond, typename _Func >	// though this is an idea, but doesn't seem of much help
	//void for_each(_Cond condition_function_takes_data_returns_direction_to_move, _Func);	// func will receive only one param, ie. the node_dtype data

public:
	/**
	* @note - It is `time based expansion`, that is the expansion rate decreases over time, and gains normal speed back up too, then again that
	*		  For more customizarion, you can overload these, for a similar example using `unused space` in the matrix to decide whether to grow,
	*		  see `world_plot.cpp` in the [WorldLine Simulator](https://github.com/adi-g15/worldLineSim) project
	* 
	* @note2 - It expands equally at each plane, again, for more customization it can be overloaded :D
	*/
	virtual void pause_auto_expansion() override;
	virtual void resume_auto_expansion() override;

	// 	OVERLOAD similar to resume_auto_expansion(), just that for each new box, the callable is executed, and returned value assigned to box::data
	template<typename Callable>
	void resume_auto_expansion(Callable&&);

	graph_box_type* operator[](const coord_type&);
	const graph_box_type* operator[](const coord_type&) const;
	graph_box_type* operator[](const graph_position& pos);
	const graph_box_type* operator[](const graph_position& pos) const;

	// these are metadata for resize() function, and it's variants
		enum class RESIZE_TYPE {
			AUTO_EXPANSION,	// called by auto expansion
			MANUAL	// manually called resize()
		};

		struct {
			RESIZE_TYPE curr_resize_type{ RESIZE_TYPE::MANUAL };

			bool add_or_inject_flag;	// when auto expanding, this is used to `alternatively` add or inject planes, so that it doesn't just expand in one direction
		} tmp_resize_data;	// this will be used by ALL size INREASING member function (so better declare than pass always)

		std::optional< Init_Func > data_initialiser;
		void set_initialiser(const Init_Func& func) { data_initialiser = func; }
		void reset_initialiser() { data_initialiser.reset(); }

		void resize(const dimen_t, const dimen_t, const dimen_t, RESIZE_TYPE = RESIZE_TYPE::MANUAL);
		void resize(const dimen_t, const dimen_t, const dimen_t, Init_Func data_initialiser);

		template<typename Func>
		void resize(const dimen_t, const dimen_t, const dimen_t, Func&&, RESIZE_TYPE = MANUAL);

	/*
		A common mistake is to declare two function templates that differ only in their default template arguments. This is illegal because default template arguments are not part of function template's signature, and declaring two different function templates with the same signature is illegal.
	*/
	template<typename _Func, std::enable_if_t<std::is_invocable_r_v<node_dtype, _Func, dimen_t, dimen_t, dimen_t>, int> = 0 >
	void for_all(_Func);	// _Func is a lambda that receives modifiable reference to the data box

	template<typename _Func, std::enable_if_t<std::is_invocable_r_v<void, _Func, node_dtype&>, int> = 0 >
	void for_all(_Func);	// _Func is a lambda that receives the 3 dimensions

	Graph_Matrix_3D();
	Graph_Matrix_3D(const coord_type& dimensions);

	~Graph_Matrix_3D();
};
