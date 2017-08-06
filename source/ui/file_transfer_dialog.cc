//
// PROJECT:         Aspia Remote Desktop
// FILE:            ui/file_transfer_dialog.cc
// LICENSE:         Mozilla Public License Version 2.0
// PROGRAMMERS:     Dmitry Chapyshev (dmitry@aspia.ru)
//

#include "ui/file_transfer_dialog.h"
#include "base/logging.h"
#include "client/file_transfer_downloader.h"
#include "client/file_transfer_uploader.h"
#include "ui/status_code.h"

#include <atlmisc.h>

namespace aspia {

UiFileTransferDialog::UiFileTransferDialog(Mode mode,
                                           std::shared_ptr<FileRequestSenderProxy> sender,
                                           const FilePath& source_path,
                                           const FilePath& target_path,
                                           const FileTransfer::FileList& file_list)
    : mode_(mode),
      sender_(std::move(sender)),
      source_path_(source_path),
      target_path_(target_path),
      file_list_(file_list)
{
    runner_ = MessageLoopProxy::Current();
    DCHECK(runner_);
    DCHECK(runner_->BelongsToCurrentThread());
}

LRESULT UiFileTransferDialog::OnInitDialog(UINT message, WPARAM wparam,
                                           LPARAM lparam, BOOL& handled)
{
    if (mode_ == Mode::UPLOAD)
    {
        file_transfer_.reset(new FileTransferUploader(sender_, this));
    }
    else
    {
        DCHECK(mode_ == Mode::DOWNLOAD);
        file_transfer_.reset(new FileTransferDownloader(sender_, this));
    }

    file_transfer_->Start(source_path_, target_path_, file_list_);

    return TRUE;
}

LRESULT UiFileTransferDialog::OnClose(UINT message, WPARAM wparam, LPARAM lparam, BOOL& handled)
{
    file_transfer_.reset();
    EndDialog(0);
    return 0;
}

LRESULT UiFileTransferDialog::OnCancelButton(WORD code, WORD ctrl_id, HWND ctrl, BOOL& handled)
{
    EndDialog(0);
    return 0;
}

void UiFileTransferDialog::OnTransferStarted(const FilePath& source_path,
                                             const FilePath& target_path,
                                             uint64_t size)
{
    if (!runner_->BelongsToCurrentThread())
    {
        runner_->PostTask(std::bind(
            &UiFileTransferDialog::OnTransferStarted, this, source_path, target_path, size));
        return;
    }

    GetDlgItem(IDC_FROM_EDIT).SetWindowTextW(source_path.c_str());
    GetDlgItem(IDC_TO_EDIT).SetWindowTextW(target_path.c_str());
}

void UiFileTransferDialog::OnTransferComplete()
{
    if (!runner_->BelongsToCurrentThread())
    {
        runner_->PostTask(std::bind(&UiFileTransferDialog::OnTransferComplete, this));
        return;
    }

    PostMessageW(WM_CLOSE);
}

void UiFileTransferDialog::OnObjectTransfer(const FilePath& object_name,
                                            uint64_t total_object_size,
                                            uint64_t left_object_size)
{
    if (!runner_->BelongsToCurrentThread())
    {
        runner_->PostTask(std::bind(&UiFileTransferDialog::OnObjectTransfer,
                                    this, object_name, total_object_size, left_object_size));
        return;
    }

    GetDlgItem(IDC_CURRENT_ITEM_EDIT).SetWindowTextW(object_name.c_str());
}

void UiFileTransferDialog::OnUnableToCreateDirectory(const FilePath& directory_path,
                                                     proto::RequestStatus status)
{
    if (!runner_->BelongsToCurrentThread())
    {
        runner_->PostTask(std::bind(
            &UiFileTransferDialog::OnUnableToCreateDirectory, this, directory_path, status));
        return;
    }

    if (!file_transfer_)
        return;

    CString message;
    message.Format(IDS_FT_OP_SEND_DIRECTORY_ERROR,
                   directory_path.c_str(),
                   RequestStatusCodeToString(status).GetBuffer(0));

    if (MessageBoxW(message, nullptr, MB_ICONQUESTION | MB_YESNO) == IDYES)
    {
        file_transfer_->OnUnableToCreateDirectoryAction(FileTransfer::Action::SKIP);
        return;
    }

    file_transfer_->OnUnableToCreateDirectoryAction(FileTransfer::Action::ABORT);
}

void UiFileTransferDialog::OnUnableToCreateFile(const FilePath& file_path,
                                                proto::RequestStatus status)
{
    if (!runner_->BelongsToCurrentThread())
    {
        runner_->PostTask(std::bind(
            &UiFileTransferDialog::OnUnableToCreateFile, this, file_path, status));
        return;
    }

    if (!file_transfer_)
        return;

    file_transfer_->OnUnableToCreateFileAction(FileTransfer::Action::SKIP);
}

void UiFileTransferDialog::OnUnableToOpenFile(const FilePath& file_path,
                                              proto::RequestStatus status)
{
    if (!runner_->BelongsToCurrentThread())
    {
        runner_->PostTask(std::bind(
            &UiFileTransferDialog::OnUnableToOpenFile,
            this, file_path, status));
        return;
    }

    if (!file_transfer_)
        return;

    file_transfer_->OnUnableToOpenFileAction(FileTransfer::Action::SKIP);
}

void UiFileTransferDialog::OnUnableToWriteFile(const FilePath& file_path,
                                               proto::RequestStatus status)
{
    if (!runner_->BelongsToCurrentThread())
    {
        runner_->PostTask(std::bind(
            &UiFileTransferDialog::OnUnableToWriteFile, this, file_path, status));
        return;
    }

    if (!file_transfer_)
        return;

    file_transfer_->OnUnableToWriteFileAction(FileTransfer::Action::SKIP);
}

void UiFileTransferDialog::OnUnableToReadFile(const FilePath& file_path,
                                              proto::RequestStatus status)
{
    if (!runner_->BelongsToCurrentThread())
    {
        runner_->PostTask(std::bind(
            &UiFileTransferDialog::OnUnableToReadFile, this, file_path, status));
        return;
    }

    if (!file_transfer_)
        return;

    file_transfer_->OnUnableToReadFileAction(FileTransfer::Action::SKIP);
}

} // namespace aspia
