#ifndef JUCI_FILESYSTEM_H_
#define JUCI_FILESYSTEM_H_
#include <vector>
#include <string>
#include <boost/filesystem.hpp>
#include <gtkmm.h>

class filesystem {
public:
  static std::string read(const std::string &path);
  static std::string read(const boost::filesystem::path &path) { return read(path.string()); }
  static int read(const std::string &path, Glib::RefPtr<Gtk::TextBuffer> text_buffer);
  static int read(const boost::filesystem::path &path, Glib::RefPtr<Gtk::TextBuffer> text_buffer) { return read(path.string(), text_buffer); }

  static int read_non_utf8(const std::string &path, Glib::RefPtr<Gtk::TextBuffer> text_buffer);
  static int read_non_utf8(const boost::filesystem::path &path, Glib::RefPtr<Gtk::TextBuffer> text_buffer) { return read_non_utf8(path.string(), text_buffer); }

  static std::vector<std::string> read_lines(const std::string &path);
  static std::vector<std::string> read_lines(const boost::filesystem::path &path) { return read_lines(path.string()); };

  static bool write(const std::string &path, const std::string &new_content);
  static bool write(const boost::filesystem::path &path, const std::string &new_content) { return write(path.string(), new_content); }
  static bool write(const std::string &path) { return write(path, ""); };
  static bool write(const boost::filesystem::path &path) { return write(path, ""); };
  static bool write(const std::string &path, Glib::RefPtr<Gtk::TextBuffer> text_buffer);
  static bool write(const boost::filesystem::path &path, Glib::RefPtr<Gtk::TextBuffer> text_buffer) { return write(path.string(), text_buffer); }

  static std::string escape_argument(const std::string &argument);
  static std::string escape_argument(const boost::filesystem::path &argument) { return escape_argument(argument.string()); };
  static std::string unescape(const std::string &argument);
  
  static bool file_in_path(const boost::filesystem::path &file_path, const boost::filesystem::path &path);
  static boost::filesystem::path find_file_in_path_parents(const std::string &file_name, const boost::filesystem::path &path);
  
  ///Attempts to get canonical path, if not, return the path parameter
  static boost::filesystem::path get_canonical_path(const boost::filesystem::path &path) noexcept;
};
#endif  // JUCI_FILESYSTEM_H_
