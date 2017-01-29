/*
 * File:   multilist_vector_queue.hpp
 * Author: Barath Kannan
 * Vector of unbounded list queues. Enqueue operations are assigned a subqueue,
 * which is used for all enqueue operations occuring from that thread. The deque
 * operation maintains a list of subqueues on which a "hit" has occured - pertaining
 * to the subqueues from which a successful dequeue operation has occured. On a
 * successful dequeue operation, the queue that is used is pushed to the front of the
 * list. The "hit lists" allow the queue to adapt fairly well to different usage contexts,
 * including when there are more readers than writers, more writers than readers,
 * and high contention.
 * Created on 25 September 2016, 12:04 AM
 */

#ifndef BK_CONQ_MULTILIST_VECTORQUEUE_HPP
#define BK_CONQ_MULTILIST_VECTORQUEUE_HPP

#include <thread>
#include <vector>
#include <numeric>
#include <bk_conq/unbounded_queue.hpp>
#include <bk_conq/list_queue.hpp>

namespace bk_conq {
template <typename T>
class multilist_vector_queue : public unbounded_queue<T> {
public:
	multilist_vector_queue(size_t subqueues) : _q(subqueues) {}
	multilist_vector_queue(const multilist_vector_queue&) = delete;
	void operator=(const multilist_vector_queue&) = delete;

	void sp_enqueue(T&& input) {
		return sp_enqueue_forward(std::move(input));
	}

	void sp_enqueue(const T& input) {
		return sp_enqueue_forward(input);
	}

	void sp_enqueue(T&& input, size_t index) {
		return sp_enqueue_forward(std::move(input), index);
	}

	void sp_enqueue(const T& input, size_t index) {
		return sp_enqueue_forward(std::move(input), index);
	}

	void mp_enqueue(T&& input) {
		return mp_enqueue_forward(std::move(input));
	}

	void mp_enqueue(const T& input) {
		return mp_enqueue_forward(input);
	}

	void mp_enqueue(T&& input, size_t index) {
		return mp_enqueue_forward(std::move(input), index);
	}

	void mp_enqueue(const T& input, size_t index) {
		return mp_enqueue_forward(input, index);
	}

	bool sc_dequeue(T& output) {
		thread_local static auto hitlist = hitlist_sequence();
		for (auto it = hitlist.begin(); it != hitlist.end(); ++it) {
			if (_q[*it].sc_dequeue(output)) {
				for (auto it2 = hitlist.begin(); it2 != it; ++it2) std::iter_swap(it, it2);
				return true;
			}
		}
		return false;
	}

	bool sc_dequeue(T& output, size_t index) {
		return _q[index].sc_dequeue(output);
	}

	bool mc_dequeue(T& output) {
		thread_local static auto hitlist = hitlist_sequence();
		for (auto it = hitlist.begin(); it != hitlist.end(); ++it) {
			if (_q[*it].mc_dequeue_light(output)) {
				for (auto it2 = hitlist.begin(); it2 != it; ++it2) std::iter_swap(it, it2);
				return true;
			}
		}
		for (auto it = hitlist.begin(); it != hitlist.end(); ++it) {
			if (_q[*it].mc_dequeue(output)) {
				for (auto it2 = hitlist.begin(); it2 != it; ++it2) std::iter_swap(it, it2);
				return true;
			}
		}
		return false;
	}

	bool mc_dequeue(T& output, size_t index) {
		return _q[index].mc_dequeue(output);
	}

private:
	std::vector<size_t> hitlist_sequence() {
		std::vector<size_t> hitlist(_q.size());
		std::iota(hitlist.begin(), hitlist.end(), 0);
		return hitlist;
	}

	template <typename U>
	void sp_enqueue_forward(U&& input) {
		thread_local static size_t indx{ _enqueue_indx.fetch_add(1) % _q.size() };
		_q[indx].mp_enqueue(std::forward<U>(input));
	}

	template <typename U>
	void sp_enqueue_forward(U&& input, size_t index) {
		_q[index].mp_enqueue(std::forward<U>(input));
	}

	template <typename U>
	void mp_enqueue_forward(U&& input) {
		thread_local static size_t indx{ _enqueue_indx.fetch_add(1) % _q.size() };
		_q[indx].mp_enqueue(std::forward<U>(input));
	}

	template <typename U>
	void mp_enqueue_forward(U&& input, size_t index) {
		_q[index].mp_enqueue(std::forward<U>(input));
	}

	class padded_list_queue : public list_queue<T> {
		char padding[64];
	};

	std::vector<padded_list_queue> _q;
	std::atomic<size_t> _enqueue_indx{ 0 };
};
}//namespace bk_conq

#endif /* BK_CONQ_MULTILIST_VECTORQUEUE_HPP */
