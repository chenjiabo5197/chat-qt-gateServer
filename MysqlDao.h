#pragma once

#include "const.h"
#include <thread>
#include <jdbc/mysql_driver.h>
#include <jdbc/mysql_connection.h>
#include <jdbc/cppconn/prepared_statement.h>
#include <jdbc/cppconn/resultset.h>
#include <jdbc/cppconn/statement.h>
#include <jdbc/cppconn/exception.h>

class SqlConnection {
public:
	SqlConnection(sql::Connection* conn, int64_t lasttime) :_conn(conn), _last_oper_time(lasttime) {}
	std::unique_ptr<sql::Connection> _conn;
	int64_t _last_oper_time;
};

class MySqlPool {
public:
	MySqlPool(const std::string& url, const std::string& user, const std::string& pass, const std::string& schema, int poolSize)
		: url_(url), user_(user), pass_(pass), schema_(schema), poolSize_(poolSize), b_stop_(false) {
		try {
			for (int i = 0; i < poolSize_; ++i) {
				sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
				auto* con = driver->connect(url_, user_, pass_);
				con->setSchema(schema_);
				// ��ȡ��ǰʱ���
				auto currentTime = std::chrono::system_clock::now().time_since_epoch();
				// ��ʱ���ת��Ϊ��
				long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();
				pool_.push(std::make_unique<SqlConnection>(con, timestamp));
			}

			_check_thread = std::thread([this]() {
				while (!b_stop_) {
					// �̳߳�û��ֹͣ�������ÿ��60s���һ��
					checkConnection();
					std::this_thread::sleep_for(std::chrono::seconds(60));
				}
				});
			// ���߳��뵱ǰ�̶߳�����룬ʹ���ں�̨���ж�������ԭ�е� std::thread ����󶨡������� detach() ��
			// �߳̽������ں�ִ̨�У����޷��ٱ����ƣ����磬������ join()��Ҳ�޷���ȡ�䷵��ֵ���쳣��
			_check_thread.detach();
		}
		catch (sql::SQLException& e) {
			// �����쳣
			std::cout << "mysql pool init failed, error is " << e.what() << std::endl;
		}
	}

	void checkConnection() {
		// ����
		/*
			1��std::unique_lock �� std::mutex ���з�װ���Զ��������Ļ�ȡ���ͷš�
			2���� lock ���󱻴���ʱ�����᳢�Ի�ȡ mutex_ ���������ǰû�������̳߳��и�����lock ��ɹ������������ִ�д��롣
			3���� lock ���󳬳�������ʱ���� lock ������ʱ����std::unique_lock ���Զ��ͷ� mutex_ �������������ǽ����Ĵ���
		*/
		std::lock_guard<std::mutex> guard(mutex_);
		int poolsize = pool_.size();
		// ��ȡ��ǰʱ���
		auto currentTime = std::chrono::system_clock::now().time_since_epoch();
		// ��ʱ���ת��Ϊ��
		long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();
		for (int i = 0; i < poolsize; i++) {
			auto con = std::move(pool_.front());
			pool_.pop();
			Defer defer([this, &con]() {
				pool_.push(std::move(con));
			});

			// ��⵱ǰʱ������ϴβ�����ʱ���֮��
			if (timestamp - con->_last_oper_time < 5) {
				continue;
			}

			try {
				std::unique_ptr<sql::Statement> stmt(con->_conn->createStatement());
				stmt->executeQuery("SELECT 1");
				con->_last_oper_time = timestamp;
				//std::cout << "execute timer alive query , cur is " << timestamp << std::endl;
			}
			catch (sql::SQLException& e) {
				std::cout << "Error keeping connection alive: " << e.what() << std::endl;
				// ���´������Ӳ��滻�ɵ�����
				sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
				auto* newcon = driver->connect(url_, user_, pass_);
				newcon->setSchema(schema_);
				con->_conn.reset(newcon);
				con->_last_oper_time = timestamp;
			}
		}
	}

	std::unique_ptr<SqlConnection> getConnection() {
		std::unique_lock<std::mutex> lock(mutex_);
		// ����falseʱ���̹߳��𣬵ȴ�notify����
		cond_.wait(lock, [this] {
			if (b_stop_) {
				return true;
			}
			return !pool_.empty(); });
		if (b_stop_) {
			return nullptr;
		}
		std::unique_ptr<SqlConnection> con(std::move(pool_.front()));
		pool_.pop();
		return con;
	}

	void returnConnection(std::unique_ptr<SqlConnection> con) {
		std::unique_lock<std::mutex> lock(mutex_);
		if (b_stop_) {
			return;
		}
		pool_.push(std::move(con));
		cond_.notify_one();
	}

	void Close() {
		b_stop_ = true;
		cond_.notify_all();
	}

	~MySqlPool() {
		std::unique_lock<std::mutex> lock(mutex_);
		while (!pool_.empty()) {
			pool_.pop();
		}
	}

private:
    // ����mysql������
    std::string url_;
    std::string user_;
    std::string pass_;
    // ���ݿ�
    std::string schema_;
    int poolSize_;
    // mysql�����Ӷ���
    std::queue<std::unique_ptr<SqlConnection>> pool_;
    // ��ֹ���̷߳��ʣ���֤��ȫ��
    std::mutex mutex_;
    // ����Ϊ��ʱ��һ�������������÷��ʵ��̹߳����뻽��
    std::condition_variable cond_;
    // ���ӳ�Ҫ�˳�ʱ����Ϊtrue
    std::atomic<bool> b_stop_;
    // ���ӳصļ���̡߳�ÿ���̶�ʱ����һ�Σ���ʱ��δ����ʱ��mysql�����󱨸���״̬����ֹmysql�Ͽ��������
    std::thread _check_thread;
};

struct UserInfo {
    std::string name;
    std::string passwd;
    int uid;
    std::string email;
};

class MysqlDao
{
public:
	MysqlDao();
	~MysqlDao();
	int RegUser(const std::string& name, const std::string& email, const std::string& pwd);
	int RegUserTransaction(const std::string& name, const std::string& email, const std::string& pwd, const std::string& icon);
	bool CheckEmail(const std::string& name, const std::string& email);
	bool UpdatePwd(const std::string& name, const std::string& newpwd);
	bool CheckPwd(const std::string& name, const std::string& pwd, UserInfo& userInfo);
	bool TestProcedure(const std::string& email, int& uid, std::string& name);
private:
	std::unique_ptr<MySqlPool> pool_;
};

