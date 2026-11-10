#include "Registration/PoseOptimizationClient.h"

FPoseOptimizationClient::FPoseOptimizationClient()
{
}

FPoseOptimizationClient::~FPoseOptimizationClient()
{
    if (Socket)
    {
        Socket->Close();
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Socket);
    }
}

bool FPoseOptimizationClient::Connect()
{
    ISocketSubsystem* S = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
    Socket = S->CreateSocket(NAME_Stream, TEXT("PoseGraphClient"), false);

    FIPv4Address Addr(127, 0, 0, 1);

    TSharedRef<FInternetAddr> Internet = S->CreateInternetAddr();
    Internet->SetIp(Addr.Value);
    Internet->SetPort(52963);

    return Socket->Connect(*Internet);
}

// -------------------------------------------------------------
// JSON senden
// -------------------------------------------------------------
bool FPoseOptimizationClient::SendJson(const FString& Json)
{
    TArray<uint8> Payload;

    // Length prefix
    uint32 Size = Json.Len();
    Payload.AddUninitialized(sizeof(uint32));
    FMemory::Memcpy(Payload.GetData(), &Size, sizeof(uint32));

    // UTF-8 JSON bytes
    FTCHARToUTF8 Convert(*Json);
    Payload.Append((uint8*)Convert.Get(), Convert.Length());

    int32 Sent;
    return Socket->Send(Payload.GetData(), Payload.Num(), Sent);
}



bool RecvAll(FSocket* Socket, uint8* Data, int32 Size)
{
    int32 Total = 0;

    while (Total < Size)
    {
        int32 BytesRead = 0;
        if (!Socket->Recv(Data + Total, Size - Total, BytesRead))
        {
            return false; // echte Fehler
        }

        if (BytesRead == 0)
        {
            // Socket ist nicht bereit – kurz warten
            FPlatformProcess::Sleep(0.001f);
            continue;
        }

        Total += BytesRead;
    }

    return true;
}


// -------------------------------------------------------------
// JSON empfangen
// -------------------------------------------------------------
bool FPoseOptimizationClient::ReceiveJson(FString& OutJson)
{
    uint32 SizeNet = 0;
    int32 Read = 0;

    if (!Socket->Recv((uint8*)&SizeNet, sizeof(uint32), Read) || Read != 4) {
        return false;
    }

    uint32 Size = SizeNet;

    TArray<uint8> Buffer;
    Buffer.SetNumUninitialized(Size + 1); // +1 für Nullterminierung

    if (!RecvAll(Socket, Buffer.GetData(), Size)) {
        UE_LOG(LogTemp, Warning, TEXT("RECV ALL ERROR."));
        return false;
    }

    // Nullterminieren
    Buffer[Size] = 0;

    OutJson = FString(UTF8_TO_TCHAR(reinterpret_cast<const char*>(Buffer.GetData())));
    return true;
}


// -------------------------------------------------------------
// PoseGraph senden + Ergebnis empfangen
// -------------------------------------------------------------
bool FPoseOptimizationClient::SendPoseGraph(
    const TArray<FPGPose>& Poses,
    const TArray<FPGEdge>& Edges,
    TArray<FPGPose>& OutOptimizedPoses)
{
    // JSON Root
    TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
    Root->SetStringField("cmd", "optimize_pose_graph");

    // Poses Array
    TArray<TSharedPtr<FJsonValue>> JPoses;
    for (const auto& P : Poses)
    {
        TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
        Obj->SetNumberField("id", P.Id);

        TArray<TSharedPtr<FJsonValue>> JT;
        JT.Reserve(16);

        for (int r=0; r<4; r++)
            for (int c=0; c<4; c++)
                JT.Add(MakeShared<FJsonValueNumber>(P.T.M[r][c]));

        Obj->SetArrayField("T", JT);
        JPoses.Add(MakeShared<FJsonValueObject>(Obj));
    }
    Root->SetArrayField("poses", JPoses);

    // Edges Array
    TArray<TSharedPtr<FJsonValue>> JEdges;
    for (const auto& E : Edges)
    {
        TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
        Obj->SetNumberField("i", E.I);
        Obj->SetNumberField("j", E.J);

        TArray<TSharedPtr<FJsonValue>> JTij;
        JTij.Reserve(16);

        for (int r=0; r<4; r++)
            for (int c=0; c<4; c++)
                JTij.Add(MakeShared<FJsonValueNumber>(E.Tij.M[r][c]));

        Obj->SetArrayField("Tij", JTij);

        JEdges.Add(MakeShared<FJsonValueObject>(Obj));
    }
    Root->SetArrayField("edges", JEdges);

    // Serialize JSON
    FString Json;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Json);
    FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);

    // Send
    if (!SendJson(Json)) {
        UE_LOG(LogTemp, Error, TEXT("SendJson failed!"));
        return false;
    }

    // Receive
    FString Resp;
    if (!ReceiveJson(Resp)) {
        UE_LOG(LogTemp, Error, TEXT("ReceiveJson failed!"));
        return false;
    }

    TSharedPtr<FJsonObject> R;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Resp);
    if (!FJsonSerializer::Deserialize(Reader, R)) {
        UE_LOG(LogTemp, Error, TEXT("Deserialize failed!"));
        UE_LOG(LogTemp, Warning, TEXT("Server raw JSON response:\n%s"), *Resp);
        return false;
    }

    if (R->GetStringField("status") != "ok") {
        UE_LOG(LogTemp, Error, TEXT("Status NOT okay!"));
        return false;
    }

    const TArray<TSharedPtr<FJsonValue>>* Opt;
    if (!R->TryGetArrayField("poses_optimized", Opt)) {
        UE_LOG(LogTemp, Error, TEXT("poses_optimized not available!"));
        return false;
    }

    OutOptimizedPoses.Empty();

    for (const auto& V : *Opt)
    {
        auto Obj = V->AsObject();

        FPGPose P;
        P.Id = Obj->GetIntegerField("id");

        const TArray<TSharedPtr<FJsonValue>>* JT;
        Obj->TryGetArrayField("T", JT);

        FMatrix44f Mat;
        int k = 0;
        for (int r = 0; r < 4; r++)
            for (int c = 0; c < 4; c++)
                Mat.M[r][c] = (*JT)[k++]->AsNumber();

        P.T = Mat;

        OutOptimizedPoses.Add(P);
    }

    return true;
}