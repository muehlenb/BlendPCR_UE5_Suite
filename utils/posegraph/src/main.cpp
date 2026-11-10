
#include <asio.hpp>
#include <iostream>
#include <vector>
#include <unordered_set>
#include <nlohmann/json.hpp>

#include <g2o/core/sparse_optimizer.h>
#include <g2o/core/block_solver.h>
#include <g2o/core/optimization_algorithm_levenberg.h>
#include <g2o/solvers/dense/linear_solver_dense.h>
#include <g2o/types/slam3d/types_slam3d.h>
#include <g2o/core/robust_kernel_impl.h>

using json = nlohmann::json;
using asio::awaitable;
using asio::use_awaitable;
using asio::co_spawn;
using asio::detached;

// -------------------------------------------------------------
// Helper: Orthonormalize rotation
// -------------------------------------------------------------
inline Eigen::Matrix3d ortho(const Eigen::Matrix3d& R_in)
{
    Eigen::JacobiSVD<Eigen::Matrix3d> svd(
        R_in, Eigen::ComputeFullU | Eigen::ComputeFullV);
    return svd.matrixU() * svd.matrixV().transpose();
}

// -------------------------------------------------------------
// Matrix helpers
// -------------------------------------------------------------
Eigen::Matrix4d readMatrix4(const json& arr)
{
    if (!arr.is_array() || arr.size() != 16)
        throw std::runtime_error("4x4 matrix must contain exactly 16 numbers");

    Eigen::Matrix4d M;
    for (int i = 0; i < 16; ++i)
        M(i / 4, i % 4) = arr[i].get<double>();

    return M;
}

json writeMatrix4(const Eigen::Matrix4d& M)
{
    json arr = json::array();
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++)
            arr.push_back(M(r, c));

    return arr;
}

// -----------------------------------------------------------------------------
// Networking Helpers (unchanged)
// -----------------------------------------------------------------------------
awaitable<void> readExact(asio::ip::tcp::socket& socket, void* data,
                          size_t size, std::chrono::milliseconds timeout)
{
    auto exec = co_await asio::this_coro::executor;
    asio::steady_timer timer(exec);

    asio::error_code ec_read;
    asio::error_code ec_timer;
    bool read_done = false;
    bool timer_done = false;

    asio::async_read(socket, asio::buffer(data, size),
                     [&](asio::error_code ec, std::size_t) {
                         ec_read = ec;
                         read_done = true;
                     });

    timer.expires_after(timeout);
    timer.async_wait([&](asio::error_code ec) {
        ec_timer = ec;
        timer_done = true;
        if (!read_done)
            socket.cancel();
    });

    while (!read_done && !timer_done)
        co_await asio::post(exec, use_awaitable);

    if (timer_done && !read_done)
        throw std::runtime_error("socket timeout");

    if (ec_read && ec_read != asio::error::operation_aborted)
        throw std::runtime_error("socket read error");

    timer.cancel();
    co_return;
}

awaitable<json> readJson(asio::ip::tcp::socket& socket)
{
    uint32_t size = 0;
    co_await readExact(socket, &size, sizeof(size), std::chrono::milliseconds(3000));

    if (size == 0 || size > 20 * 1024 * 1024)
        throw std::runtime_error("invalid json size");

    std::vector<uint8_t> buffer(size);
    co_await readExact(socket, buffer.data(), size, std::chrono::milliseconds(3000));
    co_return json::parse(buffer.begin(), buffer.end());
}

awaitable<void> sendJson(asio::ip::tcp::socket& socket, const json& j)
{
    std::string payload = j.dump();

    uint32_t size = static_cast<uint32_t>(payload.size());

    std::vector<uint8_t> buffer;
    buffer.resize(sizeof(uint32_t) + payload.size());

    std::memcpy(buffer.data(), &size, sizeof(uint32_t));
    std::memcpy(buffer.data() + sizeof(uint32_t), payload.data(), payload.size());

    co_await asio::async_write(socket, asio::buffer(buffer), use_awaitable);
}

// -----------------------------------------------------------------------------
// ★★★ REAL POSE GRAPH VERSION (OPTION 1) ★★★
// -----------------------------------------------------------------------------
json optimizePoseGraph(const json& request)
{
    json response;

    try {
        g2o::SparseOptimizer optimizer;
        optimizer.setVerbose(false);

        auto linearSolver =
            std::make_unique<g2o::LinearSolverDense<g2o::BlockSolverX::PoseMatrixType>>();

        auto blockSolver =
            std::make_unique<g2o::BlockSolverX>(std::move(linearSolver));

        optimizer.setAlgorithm(
            new g2o::OptimizationAlgorithmLevenberg(std::move(blockSolver)));

        // -------------------------------------------------
        // Vertices (Poses)
        // -------------------------------------------------
        std::unordered_set<int> ids;
        for (auto& p : request["poses"])
        {
            int id = p["id"];
            Eigen::Matrix4d M = readMatrix4(p["T"]);

            Eigen::Matrix3d R = ortho(M.block<3,3>(0,0));
            Eigen::Quaterniond q(R);
            Eigen::Vector3d t = M.block<3,1>(0,3);

            auto* v = new g2o::VertexSE3();
            v->setId(id);
            v->setEstimate(g2o::SE3Quat(q, t));
            if (id == 0)
                v->setFixed(true);

            optimizer.addVertex(v);
            ids.insert(id);
        }

        // -------------------------------------------------
        // Edges (Constraints)
        // -------------------------------------------------
        for (auto& e : request["edges"])
        {
            int i = e["i"];
            int j = e["j"];

            Eigen::Matrix4d M = readMatrix4(e["Tij"]);
            Eigen::Matrix3d R = ortho(M.block<3,3>(0,0));
            Eigen::Quaterniond q(R);
            Eigen::Vector3d t = M.block<3,1>(0,3);

            // ★ Wichtig: Jetzt korrekt interpretiert als PoseConstraint
            g2o::SE3Quat Z_ij(q, t);

            auto* edge = new g2o::EdgeSE3();
            edge->setVertex(0, optimizer.vertex(i));
            edge->setVertex(1, optimizer.vertex(j));
            edge->setMeasurement(Z_ij);

            // Information Matrix – muss stark genug sein
            Eigen::Matrix<double,6,6> info = Eigen::Matrix<double,6,6>::Identity();
            info.block<3,3>(0,0) *= 200.0;  // Rot
            info.block<3,3>(3,3) *= 20.0;   // Trans
            edge->setInformation(info);

            auto* kernel = new g2o::RobustKernelHuber();
            kernel->setDelta(0.2);
            edge->setRobustKernel(kernel);

            optimizer.addEdge(edge);
        }

        // -------------------------------------------------
        // Optimierung ausführen
        // -------------------------------------------------
        optimizer.initializeOptimization();
        optimizer.computeInitialGuess();
        optimizer.optimize(80);  // stabiler

        // -------------------------------------------------
        // Ergebnisse exportieren
        // -------------------------------------------------
        json out = json::array();
        for (int id : ids)
        {
            auto* v = (g2o::VertexSE3*)optimizer.vertex(id);
            out.push_back({
                {"id", id},
                {"T", writeMatrix4(v->estimate().matrix())}
            });
        }

        response["status"] = "ok";
        response["poses_optimized"] = out;
    }
    catch (std::exception& ex)
    {
        response["status"] = "error";
        response["message"] = ex.what();
    }

    return response;
}

// -----------------------------------------------------------------------------
// Session & listener unchanged
// -----------------------------------------------------------------------------
awaitable<void> session(asio::ip::tcp::socket socket)
{
    json response;

    try {
        json req = co_await readJson(socket);

        if (req["cmd"] == "optimize_pose_graph")
            response = optimizePoseGraph(req);
        else
            response = {{"status","error"},{"message","unknown cmd"}};
    }
    catch (const std::exception& e)
    {
        response = {{"status","error"},{"message",e.what()}};
    }

    co_await sendJson(socket, response);
}

awaitable<void> listener()
{
    auto exec = co_await asio::this_coro::executor;
    asio::ip::tcp::acceptor acceptor(exec, {asio::ip::address_v4::loopback(), 52963});
    std::cout << "PoseGraph Server listening on 52963\n";

    for (;;)
    {
        asio::ip::tcp::socket socket =
            co_await acceptor.async_accept(use_awaitable);

        co_spawn(exec, session(std::move(socket)), detached);
    }
}

int main()
{
    try {
        asio::io_context ctx(1);
        co_spawn(ctx, listener(), detached);
        ctx.run();
    }
    catch (std::exception& e)
    {
        std::cerr << "Fatal: " << e.what() << "\n";
    }
}
