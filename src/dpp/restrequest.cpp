#include <dpp/restrequest.h>

namespace dpp {

confirmation_callback_t::confirmation_callback_t() = default;
confirmation_callback_t::confirmation_callback_t(const confirmation_callback_t &) = default;
confirmation_callback_t::confirmation_callback_t(confirmation_callback_t &&) = default;
confirmation_callback_t &confirmation_callback_t::operator=(const confirmation_callback_t &) = default;
confirmation_callback_t &confirmation_callback_t::operator=(confirmation_callback_t &&) = default;
confirmation_callback_t::~confirmation_callback_t() = default;

}// namespace dpp
