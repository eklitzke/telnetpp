#include "telnetpp/options/msdp/server.hpp"
#include <gtest/gtest.h>
#include <vector>

using namespace telnetpp::literals;

namespace {

class in_an_activated_msdp_server : public testing::Test
{
protected:
    in_an_activated_msdp_server()
    {
        server_.on_receive.connect(
            [this](telnetpp::options::msdp::variable const &var,
                   telnetpp::options::msdp::server::continuation const &cont)
            {
                received_variables_.push_back(var);
            });
            
        server_.negotiate(telnetpp::do_, [](auto &&){});
        assert(server_.active());
    }
    
    telnetpp::options::msdp::server server_;
    std::vector<telnetpp::options::msdp::variable> received_variables_;
};

}

TEST(msdp_server_test, option_is_msdp)
{
    telnetpp::options::msdp::server server;
    ASSERT_EQ(69, server.option_code());
}

TEST_F(in_an_activated_msdp_server, send_with_variable_sends_simple_variable)
{
    telnetpp::options::msdp::variable const var{"var"_tb, "val"_tb};

    server_.send(
        var,
        [this](telnetpp::elements data)
        {
            ASSERT_EQ(size_t{1}, data.size());

            auto const expected_content = "\x01" "var" "\x02" "val"_tb;
            auto const expected_subnegotiation = telnetpp::subnegotiation{
                server_.option_code(),
                expected_content
            };

            ASSERT_EQ(telnetpp::element{expected_subnegotiation}, data[0]);
        });
}

TEST_F(in_an_activated_msdp_server, send_with_array_sends_array_variable)
{
    server_.send(
        telnetpp::options::msdp::variable{
            "var"_tb,
            telnetpp::options::msdp::array_value{ "val0"_tb, "val1"_tb }
        },
        [this](telnetpp::elements data)
        {
            ASSERT_EQ(size_t{1}, data.size());

            auto const expected_content = 
                "\x01" "var" 
                "\x02"
                    "\x05" 
                        "\x02" "val0"
                        "\x02" "val1"
                    "\x06"_tb;
        
            auto const expected_subnegotiation = telnetpp::subnegotiation{
                server_.option_code(),
                expected_content
            };

            ASSERT_EQ(telnetpp::element{expected_subnegotiation}, data[0]);
        });
}

TEST_F(in_an_activated_msdp_server, send_with_table_sends_table_variable)
{
    server_.send(
        telnetpp::options::msdp::variable{
            "var"_tb,
            telnetpp::options::msdp::table_value{
                telnetpp::options::msdp::variable{
                    "tbl"_tb,
                    telnetpp::options::msdp::array_value{
                        "val0"_tb, "val1"_tb
                    }
                }
            }
        },
        [this](telnetpp::elements data)
        {
            ASSERT_EQ(size_t{1}, data.size());

            auto const expected_content = 
                "\x01" "var" 
                "\x02"
                    "\x03" 
                        "\x01" "tbl"
                        "\x02"
                            "\x05" 
                                "\x02" "val0"
                                "\x02" "val1"
                            "\x06"
                    "\x04"_tb;
        
            auto const expected_subnegotiation = telnetpp::subnegotiation{
                server_.option_code(),
                expected_content
            };

            ASSERT_EQ(telnetpp::element{expected_subnegotiation}, data[0]);
        });
}

TEST_F(in_an_activated_msdp_server, receiving_no_variables_does_nothing)
{
    server_.subnegotiate({}, [](auto &&){});

    ASSERT_EQ(size_t{0}, received_variables_.size());
}

TEST_F(in_an_activated_msdp_server, receiving_a_variable_reports_an_array_of_one_variable)
{
    auto const subnegotiation_content =
        "\x01" "var"
        "\x02" "val"_tb;
        
    server_.subnegotiate(subnegotiation_content, [](auto &&){});

    auto const expected = telnetpp::options::msdp::variable{"var"_tb, "val"_tb};

    ASSERT_EQ(size_t{1}, received_variables_.size());
    ASSERT_EQ(expected, received_variables_[0]);
}

TEST_F(in_an_activated_msdp_server, receiving_two_variables_reports_two_variable)
{
    auto const subnegotiation_content =
        "\x01" "var0"
        "\x02" "val0"
        "\x01" "var1"
        "\x02" "val1"_tb;
        
    server_.subnegotiate(subnegotiation_content, [](auto &&){});

    auto const expected0 = telnetpp::options::msdp::variable{"var0"_tb, "val0"_tb};
    auto const expected1 = telnetpp::options::msdp::variable{"var1"_tb, "val1"_tb};

    ASSERT_EQ(size_t{2}, received_variables_.size());
    ASSERT_EQ(expected0, received_variables_[0]);
    ASSERT_EQ(expected1, received_variables_[1]);
}

TEST_F(in_an_activated_msdp_server, receiving_empty_array_variable_reports_empty_array)
{
    auto const subnegotiation_content =
        "\x01" "arr"
        "\x02" "\x05"
               "\x06"_tb;
        
    server_.subnegotiate(subnegotiation_content, [](auto &&){});

    auto const expected = telnetpp::options::msdp::variable{
        "arr"_tb, 
        telnetpp::options::msdp::array_value{}
    };

    ASSERT_EQ(size_t{1}, received_variables_.size());
    ASSERT_EQ(expected, received_variables_[0]);
}

TEST_F(in_an_activated_msdp_server, receiving_array_variable_with_one_element_reports_array)
{
    auto const subnegotiation_content =
        "\x01" "arr"
        "\x02" "\x05"
                      "\x02" "val"
               "\x06"_tb;
        
    server_.subnegotiate(subnegotiation_content, [](auto &&){});

    auto const expected = telnetpp::options::msdp::variable{
        "arr"_tb, 
        telnetpp::options::msdp::array_value{ "val"_tb }
    };

    ASSERT_EQ(size_t{1}, received_variables_.size());
    ASSERT_EQ(expected, received_variables_[0]);
}

TEST_F(in_an_activated_msdp_server, receiving_array_variable_with_two_elements_reports_array)
{
    auto const subnegotiation_content =
        "\x01" "arr"
        "\x02" "\x05"
                      "\x02" "val0"
                      "\x02" "val1"
               "\x06"_tb;

    server_.subnegotiate(subnegotiation_content, [](auto &&){});

    auto const expected = telnetpp::options::msdp::variable{
        "arr"_tb, 
        telnetpp::options::msdp::array_value{ "val0"_tb, "val1"_tb }
    };

    ASSERT_EQ(size_t{1}, received_variables_.size());
    ASSERT_EQ(expected, received_variables_[0]);
}

TEST_F(in_an_activated_msdp_server, receiving_array_variable_then_string_reports_array_and_string)
{
    auto const subnegotiation_content =
        "\x01" "arr"
        "\x02" "\x05"
                      "\x02" "val0"
                      "\x02" "val1"
               "\x06"
        "\x01" "var"
        "\x02" "val"_tb;

    server_.subnegotiate(subnegotiation_content, [](auto &&){});

    auto const expected0 = telnetpp::options::msdp::variable{
        "arr"_tb,
        telnetpp::options::msdp::array_value{ "val0"_tb, "val1"_tb }
    };

    auto const expected1 = telnetpp::options::msdp::variable{"var"_tb, "val"_tb};

    ASSERT_EQ(size_t{2}, received_variables_.size());
    ASSERT_EQ(expected0, received_variables_[0]);
    ASSERT_EQ(expected1, received_variables_[1]);
}

TEST_F(in_an_activated_msdp_server, receiving_empty_table_reports_empty_table)
{
    auto const subnegotiation_content =
        "\x01" "tbl"
        "\x02" "\x03"
               "\x04"_tb;

    server_.subnegotiate(subnegotiation_content, [](auto &&){});

    auto const expected = telnetpp::options::msdp::variable{
        "tbl"_tb,
        telnetpp::options::msdp::table_value{}
    };

    ASSERT_EQ(size_t{1}, received_variables_.size());
    ASSERT_EQ(expected, received_variables_[0]);
}

TEST_F(in_an_activated_msdp_server, receiving_table_with_one_string_value_returns_table_with_value)
{
    auto const subnegotiation_content =
        "\x01" "tbl"
        "\x02" "\x03"
                      "\x01" "var"
                      "\x02" "val"
               "\x04"_tb;

    server_.subnegotiate(subnegotiation_content, [](auto &&){});

    auto const expected = telnetpp::options::msdp::variable{
        "tbl"_tb,
        telnetpp::options::msdp::table_value{
            telnetpp::options::msdp::variable{"var"_tb, "val"_tb}
        }
    };

    ASSERT_EQ(size_t{1}, received_variables_.size());
    ASSERT_EQ(expected, received_variables_[0]);
}

TEST_F(in_an_activated_msdp_server, receiving_table_with_one_array_value_returns_table_with_value)
{
    auto const subnegotiation_content =
        "\x01" "tbl"
        "\x02" "\x03"
                      "\x01" "arr"
                      "\x02" "\x05" 
                                    "\x02" "val0"
                                    "\x02" "val1"
                                    "\x02" "val2"
                             "\x06"
               "\x04"_tb;

    server_.subnegotiate(subnegotiation_content, [](auto &&){});

    auto const expected = telnetpp::options::msdp::variable{
        "tbl"_tb,
        telnetpp::options::msdp::table_value{
            telnetpp::options::msdp::variable{
                "arr"_tb,
                telnetpp::options::msdp::array_value{ 
                    "val0"_tb, "val1"_tb, "val2"_tb
                }
            }
        }
    };

    ASSERT_EQ(size_t{1}, received_variables_.size());
    ASSERT_EQ(expected, received_variables_[0]);
}

TEST_F(in_an_activated_msdp_server, receiving_table_with_one_table_value_returns_table_with_value)
{
    auto const subnegotiation_content =
        "\x01" "tbl"
        "\x02" "\x03"
                      "\x01" "in"
                      "\x02" "\x03" 
                                    "\x01" "var"
                                    "\x02" "val"
                             "\x04"
               "\x04"_tb;

    server_.subnegotiate(subnegotiation_content, [](auto &&){});

    auto const expected = telnetpp::options::msdp::variable{
        "tbl"_tb,
        telnetpp::options::msdp::table_value{
            telnetpp::options::msdp::variable{
                "in"_tb,
                telnetpp::options::msdp::table_value{
                    telnetpp::options::msdp::variable{"var"_tb, "val"_tb}
                }
            }
        }
    };

    ASSERT_EQ(size_t{1}, received_variables_.size());
    ASSERT_EQ(expected, received_variables_[0]);
}

TEST_F(in_an_activated_msdp_server, receiving_table_with_many_values_returns_table_with_values)
{
    auto const subnegotiation_content =
        "\x01" "tbl"
        "\x02" "\x03"
                      "\x01" "var"
                      "\x02" "\x03"
                                    "\x01" "in"
                                    "\x02" "\x05" 
                                                  "\x02" "var"
                                                  "\x02" "val"
                                           "\x06"
                             "\x04"
                      "\x01" "var1"
                      "\x02" "val1"
                      "\x01" "var2"
                      "\x02" "\x05"
                                    "\x02" "arr0"
                                    "\x02" "arr1"
                             "\x06"
                "\x04"_tb;
                
    server_.subnegotiate(subnegotiation_content, [](auto &&){});
    
    auto const expected = telnetpp::options::msdp::variable{
        "tbl"_tb,
        telnetpp::options::msdp::table_value{
            telnetpp::options::msdp::variable{
                "var"_tb,
                telnetpp::options::msdp::table_value{
                    telnetpp::options::msdp::variable{
                        "in"_tb,
                        telnetpp::options::msdp::array_value{
                            "var"_tb, "val"_tb
                        }
                    }
                }
            },
            telnetpp::options::msdp::variable{"var1"_tb, "val1"_tb},
            telnetpp::options::msdp::variable{
                "var2"_tb,
                telnetpp::options::msdp::array_value{"arr0"_tb, "arr1"_tb}
            }
        }
    };

    ASSERT_EQ(size_t{1}, received_variables_.size());
    ASSERT_EQ(expected, received_variables_[0]);
}
